#include "cache.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>



static int pt_cache_new (struct pt_cache **cache_ptr, const char *path, int mode)
{
    struct pt_cache *cache;

    // alloc
    if ((cache = calloc(1, sizeof(*cache))) == NULL)
        return -1;

    if ((cache->path = strdup(path)) == NULL)
        goto error;

    // init
    cache->fd = -1;
    cache->mode = mode;

    // ok
    *cache_ptr = cache;

    return 0;

error:
    // cleanup
    if (cache)
        pt_cache_destroy(cache);

    return -1;
}

int pt_cache_open (struct pt_cache **cache_ptr, const char *path, int mode)
{
    struct pt_cache *cache;
    
    // alloc
    if (pt_cache_new(&cache, path, mode))
        return -1;
    
    // ok
    *cache_ptr = cache;

    return 0;
}

int pt_cache_stale (struct pt_cache *cache, const char *img_path)
{
    struct stat st_img, st_cache;
    
    // test original file
    if (stat(img_path, &st_img) < 0)
        return -1;
    
    // test cache file
    if (stat(cache->path, &st_cache) < 0) {
        // always stale if it doesn't exist yet
        if (errno == ENOENT)
            return 1;
        else
            return -1;
    }

    // compare mtime
    return (st_img.st_mtime > st_cache.st_mtime);
}

/**
 * Abort any incomplete open operation, cleaning up
 */
static void pt_cache_abort (struct pt_cache *cache)
{
    if (cache->mmap != NULL) {
        munmap(cache->mmap, cache->size);

        cache->mmap = NULL;
    }

    if (cache->fd >= 0) {
        close(cache->fd);

        cache->fd = -1;
    }
}

/**
 * Open the cache file as an fd.
 *
 * XXX: needs locking
 */
static int pt_cache_open_fd (struct pt_cache *cache, int *fd_ptr)
{
    int fd;
    int flags = 0;

    // determine open flags
    // XXX: O_RDONLY | O_WRONLY == O_RDWR?
    if (cache->mode & PT_IMG_READ)
        flags |= O_RDONLY;

    if (cache->mode & PT_IMG_WRITE)
        flags |= (O_WRONLY | O_CREAT);

    // actual open()
    if ((fd = open(cache->path, flags)) < 0)
        return -1;

    // ok
    *fd_ptr = fd;

    return 0;
}

/**
 * Mmap the opened cache file from offset PT_CACHE_PAGE, using the calculated size stored in cache->size
 */
static int pt_cache_open_mmap (struct pt_cache *cache, void **addr_ptr)
{
    int prot = 0;
    void *addr;

    // determine prot
    if (cache->mode & PT_IMG_READ)
        prot |= PROT_READ;

    if (cache->mode & PT_IMG_WRITE)
        prot |= PROT_WRITE;

    // perform mmap() from second page on
    if ((addr = mmap(NULL, cache->size, prot, MAP_SHARED, cache->fd, PT_CACHE_PAGE)) == MAP_FAILED)
        return -1;

    // ok
    *addr_ptr = addr;

    return 0;
}

/**
 * Write out the cache header into the opened file
 */
static int pt_cache_write_header (struct pt_cache *cache, const struct pt_cache_header *header)
{
    size_t len = sizeof(*header);
    const char *buf = (const char *) header;

    // seek to start
    if (lseek(cache->fd, 0, SEEK_SET) != 0)
        return -1;

    // write out full header
    while (len) {
        ssize_t ret;
        
        // try and write out the header
        if ((ret = write(cache->fd, buf, len)) < 0)
            return -1;

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Create a new cache file, open it, and write out the header.
 */
static int pt_cache_open_create (struct pt_cache *cache, struct pt_cache_header *header)
{
    // no access
    if (!(cache->mode & PT_IMG_WRITE)) {
        errno = EPERM;
        return -1;
    }

    // open
    if (pt_cache_open_fd(cache, &cache->fd))
        return -1;

    // calculate data size
    cache->size = sizeof(*header) + header->height * header->row_bytes;

    // grow file
    if (ftruncate(cache->fd, PT_CACHE_PAGE + cache->size) < 0)
        goto error;

    // open mmap
    if (pt_cache_open_mmap(cache, (void **) &cache->mmap))
        goto error;

    // write header
    if (pt_cache_write_header(cache, header))
        goto error;

    // done
    return 0;

error:
    // cleanup
    pt_cache_abort(cache);

    return -1;
}

int pt_cache_update_png (struct pt_cache *cache, png_structp png, png_infop info)
{
    struct pt_cache_header header;
    
    // XXX: check cache_mode
    // XXX: check image doesn't use any options we don't handle

    memset(&header, 0, sizeof(header));

    // fill in basic info
    header.width = png_get_image_width(png, info);
    header.height = png_get_image_height(png, info);
    header.bit_depth = png_get_bit_depth(png, info);
    header.color_type = png_get_color_type(png, info);

    // fill in other info
    header.row_bytes = png_get_rowbytes(png, info);

    // create and write out header
    if (pt_cache_open_create(cache, &header))
        return -1;

    // XXX: pallette etc.

    // write out raw image data a row at a time
    for (size_t row = 0; row < header.height; row++) {
        // read row data, non-interlaced
        png_read_row(png, cache->mmap + row * header.row_bytes, NULL);
    }

    // done!
    return 0;
}

void pt_cache_destroy (struct pt_cache *cache)
{
    free(cache->path);

    pt_cache_abort(cache);

    free(cache);
}
