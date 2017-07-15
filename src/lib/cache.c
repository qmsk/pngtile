#include "cache.h"
#include "shared/util.h"
#include "shared/log.h" // only LOG_DEBUG
#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>


int pt_cache_new (struct pt_cache **cache_ptr, const char *path, int mode)
{
    struct pt_cache *cache;
    int err;

    // alloc
    if ((cache = calloc(1, sizeof(*cache))) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    if ((cache->path = strdup(path)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_MEM);

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

    return err;
}

/**
 * Force-clean pt_cache, warn on errors
 */
static void pt_cache_abort (struct pt_cache *cache)
{
    if (cache->file != NULL) {
        if (munmap(cache->file, sizeof(struct pt_cache_file) + cache->file->header.data_size))
            log_warn_errno("munmap: %p, %zu", cache->file, sizeof(struct pt_cache_file) + cache->file->header.data_size);

        cache->file = NULL;
    }

    if (cache->fd >= 0) {
        if (close(cache->fd))
            log_warn_errno("close: %d", cache->fd);

        cache->fd = -1;
    }
}

/**
 * Open the cache file as an fd for reading
 */
static int pt_cache_open_read_fd (struct pt_cache *cache, int *fd_ptr)
{
    int fd;

    // actual open()
    if ((fd = open(cache->path, O_RDONLY)) < 0)
        RETURN_ERROR_ERRNO(PT_ERR_OPEN_MODE, EACCES);

    // ok
    *fd_ptr = fd;

    return 0;
}

/**
 * Read in the cache header from the open file
 */
static int pt_cache_read_header (int fd, struct pt_cache_header *header)
{
    size_t len = sizeof(*header);
    char *buf = (char *) header;

    // seek to start
    if (lseek(fd, 0, SEEK_SET) != 0)
        RETURN_ERROR(PT_ERR_CACHE_SEEK);

    // write out full header
    while (len) {
        ssize_t ret;

        // try and write out the header
        if ((ret = read(fd, buf, len)) <= 0)
            RETURN_ERROR(PT_ERR_CACHE_READ);

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Read and return the version number from the cache file, temporarily opening it if needed
 */
static int pt_cache_version (struct pt_cache *cache)
{
    int fd;
    struct pt_cache_header header;
    int ret;

    // already open?
    if (cache->file)
        return cache->file->header.version;

    // temp. open
    if ((ret = pt_cache_open_read_fd(cache, &fd)))
        return ret;

    // read header
    if ((ret = pt_cache_read_header(fd, &header)))
        JUMP_ERROR(ret);

    // ok
    ret = header.version;

error:
    // close
    close(fd);

    return ret;
}

int pt_cache_status (struct pt_cache *cache, const char *img_path)
{
    struct stat st_img, st_cache;
    int ver;

    // test original file
    if (stat(img_path, &st_img) < 0)
        RETURN_ERROR(PT_ERR_IMG_STAT);

    // test cache file
    if (stat(cache->path, &st_cache) < 0) {
        // always stale if it doesn't exist yet
        if (errno == ENOENT)
            return PT_CACHE_NONE;
        else
            RETURN_ERROR(PT_ERR_CACHE_STAT);
    }

    // compare mtime
    if (st_img.st_mtime > st_cache.st_mtime)
        return PT_CACHE_STALE;

    // read version
    if ((ver = pt_cache_version(cache)) < 0)
        // fail
        return ver;

    // compare version
    if (ver != PT_CACHE_VERSION)
        return PT_CACHE_INCOMPAT;

    // ok, should be in order
    return PT_CACHE_FRESH;
}

void pt_cache_info (struct pt_cache *cache, struct pt_image_info *info)
{
    struct stat st;

    if (cache->file)
        // img info
        pt_png_info(&cache->file->header.png, info);

    // stat
    if (stat(cache->path, &st) < 0) {
        // unknown
        info->cache_mtime = 0;
        info->cache_bytes = 0;
        info->cache_blocks = 0;

    } else {
        // store
        info->cache_version = pt_cache_version(cache);
        info->cache_mtime = st.st_mtime;
        info->cache_bytes = st.st_size;
        info->cache_blocks = st.st_blocks;
    }
}

static int pt_cache_tmp_name (struct pt_cache *cache, char tmp_path[], size_t tmp_len)
{
    // get .tmp path
    if (path_with_fext(cache->path, tmp_path, tmp_len, ".tmp"))
        RETURN_ERROR(PT_ERR_PATH);

    return 0;
}

/**
 * Compute and return the full size of the .cache file
 */
static size_t pt_cache_size (size_t data_size)
{
    assert(sizeof(struct pt_cache_file) == PT_CACHE_HEADER_SIZE);

    return sizeof(struct pt_cache_file) + data_size;
}

/**
 * Open the .tmp cache file as an fd for writing
 */
static int pt_cache_open_tmp_fd (struct pt_cache *cache, int *fd_ptr)
{
    int fd;
    char tmp_path[1024];
    int err;

    // get .tmp path
    if ((err = pt_cache_tmp_name(cache, tmp_path, sizeof(tmp_path))))
        return err;

    // open for write, create, fail if someone else already opened it for update
    if ((fd = open(tmp_path, O_RDWR | O_CREAT | O_EXCL, 0644)) < 0)
        RETURN_ERROR(PT_ERR_CACHE_OPEN_TMP);

    // ok
    *fd_ptr = fd;

    return 0;
}


/**
 * Mmap the pt_cache_file using pt_cache_size(data_size)
 */
static int pt_cache_open_mmap (struct pt_cache *cache, struct pt_cache_file **file_ptr, size_t data_size, bool readonly)
{
    int prot = 0;
    void *addr;

    // determine prot
    prot |= PROT_READ;

    if (!readonly) {
        assert(cache->mode & PT_OPEN_UPDATE);

        prot |= PROT_WRITE;
    }

    // mmap() the full file including header
    if ((addr = mmap(NULL, pt_cache_size(data_size), prot, MAP_SHARED, cache->fd, 0)) == MAP_FAILED)
        RETURN_ERROR(PT_ERR_CACHE_MMAP);

    // ok
    *file_ptr = addr;

    return 0;
}

int pt_cache_open (struct pt_cache *cache)
{
    struct pt_cache_header header;
    int err;

    // ignore if already open
    if (cache->file)
        return 0;

    // open the .cache in readonly mode
    if ((err = pt_cache_open_read_fd(cache, &cache->fd)))
        return err;

    // read in header
    if ((err = pt_cache_read_header(cache->fd, &header)))
        JUMP_ERROR(err);

    // check version
    if (header.version != PT_CACHE_VERSION)
        JUMP_SET_ERROR(err, PT_ERR_CACHE_VERSION);

    // mmap the header + data
    if ((err = pt_cache_open_mmap(cache, &cache->file, header.data_size, true)))
        JUMP_ERROR(err);

    // done
    return 0;

error:
    // cleanup
    pt_cache_abort(cache);

    return err;
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
        RETURN_ERROR(PT_ERR_CACHE_SEEK);

    // write out full header
    while (len) {
        ssize_t ret;

        // try and write out the header
        if ((ret = write(cache->fd, buf, len)) <= 0)
            RETURN_ERROR(PT_ERR_CACHE_WRITE);

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Create a new .tmp cache file, open it, and write out the header.
 */
static int pt_cache_create (struct pt_cache *cache, struct pt_cache_header *header)
{
    int err;

    assert(cache->mode & PT_OPEN_UPDATE);

    // open as .tmp
    if ((err = pt_cache_open_tmp_fd(cache, &cache->fd)))
        return err;

    // write header
    if ((err = pt_cache_write_header(cache, header)))
        JUMP_ERROR(err);

    // grow file
    if (ftruncate(cache->fd, pt_cache_size(header->data_size)) < 0)
        JUMP_SET_ERROR(err, PT_ERR_CACHE_TRUNC);

    // mmap header and data
    if ((err = pt_cache_open_mmap(cache, &cache->file, header->data_size, false)))
        JUMP_ERROR(err);

    // done
    return 0;

error:
    // cleanup
    pt_cache_abort(cache);

    return err;
}

/**
 * Rename the opened .tmp to .cache
 */
static int pt_cache_create_done (struct pt_cache *cache)
{
    char tmp_path[1024];
    int err;

    // get .tmp path
    if ((err = pt_cache_tmp_name(cache, tmp_path, sizeof(tmp_path))))
        return err;

    // rename
    if (rename(tmp_path, cache->path) < 0)
        RETURN_ERROR(PT_ERR_CACHE_RENAME_TMP);

    // ok
    return 0;
}

/**
 * Abort a failed cache update after cache_create
 */
static void pt_cache_create_abort (struct pt_cache *cache)
{
    char tmp_path[1024];
    int err;

    // close open stuff
    pt_cache_abort(cache);

    // get .tmp path
    if ((err = pt_cache_tmp_name(cache, tmp_path, sizeof(tmp_path)))) {
        log_warn_errno("pt_cache_tmp_name: %s: %s", cache->path, pt_strerror(err));

        return;
    }

    // remove .tmp
    if (unlink(tmp_path))
        log_warn_errno("unlink: %s", tmp_path);
}

int pt_cache_update (struct pt_cache *cache, struct pt_png_img *img, const struct pt_image_params *params)
{
    struct pt_cache_header header;
    int err;

    // check mode
    if (!(cache->mode & PT_OPEN_UPDATE))
        RETURN_ERROR(PT_ERR_OPEN_MODE);

    // close if open
    if ((err = pt_cache_close(cache)))
        return err;

    // prep header
    header.version = PT_CACHE_VERSION;
    header.format = PT_IMG_PNG;

    // read img header
    if ((err = pt_png_read_header(img, &header.png, &header.data_size)))
        return err;

    // save any params
    if (params)
        header.params = *params;

    // create/open .tmp and write out header
    if ((err = pt_cache_create(cache, &header)))
        return err;

    // decode to disk
    if ((err = pt_png_decode(img, &cache->file->header.png, &cache->file->header.params, cache->file->data)))
        goto error;

    // done, commit .tmp
    if ((err = pt_cache_create_done(cache)))
        goto error;

    return 0;

error:
    // cleanup .tmp
    pt_cache_create_abort(cache);

    return err;
}

int pt_cache_tile (struct pt_cache *cache, struct pt_tile *tile)
{
    int err;

    // ensure open
    if ((err = pt_cache_open(cache)))
        return err;

    // render
    if ((err = pt_png_tile(&cache->file->header.png, cache->file->data, tile)))
        return err;

    return 0;
}

int pt_cache_close (struct pt_cache *cache)
{
    if (cache->file != NULL) {
        if (munmap(cache->file, sizeof(struct pt_cache_file) + cache->file->header.data_size))
            RETURN_ERROR(PT_ERR_CACHE_MUNMAP);

        cache->file = NULL;
    }

    if (cache->fd >= 0) {
        if (close(cache->fd))
            RETURN_ERROR(PT_ERR_CACHE_CLOSE);

        cache->fd = -1;
    }

    return 0;
}

void pt_cache_destroy (struct pt_cache *cache)
{
    // cleanup
    pt_cache_abort(cache);

    free(cache->path);
    free(cache);
}
