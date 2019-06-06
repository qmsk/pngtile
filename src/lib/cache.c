#include "cache.h"
#include "log.h"
#include "path.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>

const uint16_t pt_cache_version = PT_CACHE_VERSION;
const uint8_t pt_cache_magic[6] = PT_CACHE_MAGIC;

/**
 * Compute and return the full size of the .cache file
 */
static inline size_t sizeof_pt_cache_file (size_t data_size)
{
    assert(sizeof(struct pt_cache_file) == PT_CACHE_HEADER_SIZE);

    return sizeof(struct pt_cache_file) + data_size;
}

int pt_cache_path (const char *path, char *buf, size_t len)
{
    if (pt_path_make_ext(buf, len, path, ".cache")) {
        return -PT_ERR_PATH;
    }

    return 0;
}

/**
 * Open the cache file as an fd for reading
 */
static int pt_open_cache_read_fd (const char *path, int *fd_ptr)
{
    PT_DEBUG("%s", path);

    int fd;

    // actual open()
    if ((fd = open(path, O_RDONLY)) < 0)
        return -PT_ERR_CACHE_OPEN_READ;

    // ok
    *fd_ptr = fd;

    return 0;
}

/**
 * Read in the cache header from the open file
 */
static int pt_cache_header_read (struct pt_cache_header *header, int fd)
{
    size_t len = sizeof(*header);
    char *buf = (char *) header;

    // seek to start
    if (lseek(fd, 0, SEEK_SET) != 0)
        return -PT_ERR_CACHE_SEEK;

    // write out full header
    while (len) {
        ssize_t ret;

        // try and write out the header
        if ((ret = read(fd, buf, len)) <= 0)
            return -PT_ERR_CACHE_READ;

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Write out the cache header into the opened file
 */
static int pt_cache_header_write (const struct pt_cache_header *header, int fd)
{
    size_t len = sizeof(*header);
    const char *buf = (const char *) header;

    // seek to start
    if (lseek(fd, 0, SEEK_SET) != 0)
        return -PT_ERR_CACHE_SEEK;

    // write out full header
    while (len) {
        ssize_t ret;

        // try and write out the header
        if ((ret = write(fd, buf, len)) <= 0)
            return -PT_ERR_CACHE_WRITE;

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Validate header: magic, version, format
 */
static int pt_cache_header_check (const struct pt_cache_header *header)
{
  PT_DEBUG("magic=%6.6s version=%d format=%d", header->magic, header->version, header->format);

  if (memcmp(header->magic, pt_cache_magic, sizeof(pt_cache_magic)) != 0) {
      return -PT_ERR_CACHE_MAGIC;
  }

  if (header->version != pt_cache_version)
      return -PT_ERR_CACHE_VERSION;

  switch (header->format) {
    case PT_FORMAT_CACHE:
      return -PT_ERR_CACHE_FORMAT;
    case PT_FORMAT_PNG:
      break;
    default:
      return -PT_ERR_CACHE_FORMAT;
  }

  return 0;
}

static int pt_read_cache_header (const char *path, struct pt_cache_header *header)
{
  int fd;
  int err;

  if ((err = pt_open_cache_read_fd(path, &fd)))
      return err;

  if ((err = pt_cache_header_read(header, fd)))
      goto error; // XXX: return 1 on EOF / short header?

error:
  close(fd);

  return err;
}

int pt_check_cache (const char *path)
{
  struct pt_cache_header header;
  int ret;

  if ((ret = pt_read_cache_header(path, &header)))
    return ret;

  if ((ret = pt_cache_header_check(&header)))
    return ret;

  return 0;
}

int pt_stat_cache (const char *path, const char *img_path)
{
    PT_DEBUG("%s <=> %s", path, img_path);

    struct pt_cache_header header;
    struct stat st_img, st_cache;
    int err = 0;

    // test original file
    if (stat(img_path, &st_img) < 0)
        return -PT_ERR_IMG_STAT;

    // test cache file
    if (stat(path, &st_cache) < 0) {
        // always stale if it doesn't exist yet
        if (errno == ENOENT)
            return PT_CACHE_NONE;
        else
            return -PT_ERR_CACHE_STAT;
    }

    // compare mtime
    if (st_img.st_mtime > st_cache.st_mtime)
        return PT_CACHE_STALE;

    // read header
    if ((err = pt_read_cache_header(path, &header))) {
        return err;
    }

    // check version, magic, format
    if ((err = pt_cache_header_check(&header)))
        return PT_CACHE_INCOMPAT;

    // ok, should be in order
    return PT_CACHE_FRESH;
}

int pt_read_cache_info (const char *path, struct pt_image_info *info)
{
    struct pt_cache_header header;
    int err = 0;
    struct stat st;

    if (stat(path, &st) < 0) {
      return -PT_ERR_CACHE_STAT;
    }

    // cache file info
    info->cache.mtime = st.st_mtime;
    info->cache.bytes = st.st_size;
    info->cache.blocks = st.st_blocks;

    // read header
    if ((err = pt_read_cache_header(path, &header)))
        return err;

    info->cache.version = header.version;

    // image info
    info->format = header.format;

    switch (header.format) {
      case PT_FORMAT_CACHE:
        return -PT_ERR_CACHE_FORMAT;

      case PT_FORMAT_PNG:
        info->width = header.png.width;
        info->height = header.png.height;
        info->bpp = header.png.bit_depth;

        break;

    }

    return 0;
}

int pt_cache_new (struct pt_cache **cache_ptr, const char *path)
{
    PT_DEBUG("%s", path);

    struct pt_cache *cache;
    int err;

    // alloc
    if ((cache = calloc(1, sizeof(*cache))) == NULL)
        return -PT_ERR_MEM;

    if ((cache->path = strdup(path)) == NULL) {
        err = -PT_ERR_MEM;
        goto error;
    }

    // init
    cache->fd = -1;

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
    PT_DEBUG("%s", cache->path);

    if (cache->file != NULL) {
        if (munmap(cache->file, sizeof_pt_cache_file(cache->file->header.data_size)))
            PT_WARN_ERRNO("munmap %p, %zu", cache->file, sizeof_pt_cache_file(cache->file->header.data_size));

        cache->file = NULL;
    }

    if (cache->fd >= 0) {
        if (close(cache->fd))
            PT_WARN_ERRNO("close %d", cache->fd);

        cache->fd = -1;
    }
}

static int pt_cache_tmp_name (struct pt_cache *cache, char tmp_path[], size_t tmp_len)
{
    // get .tmp path
    if (pt_path_make_ext(tmp_path, tmp_len, cache->path, ".tmp"))
        return -PT_ERR_PATH;

    return 0;
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

    // replace any old .tmp file
    if (unlink(tmp_path) < 0 && errno != ENOENT)
        return -PT_ERR_CACHE_UNLINK_TMP;

    // open for write, create, fail if someone else already opened it for update
    if ((fd = open(tmp_path, O_RDWR | O_CREAT | O_EXCL, 0644)) < 0)
        return -PT_ERR_CACHE_OPEN_TMP;

    // ok
    *fd_ptr = fd;

    return 0;
}


/**
 * Mmap the pt_cache_file using sizeof_pt_cache_file(data_size)
 */
static int pt_cache_open_mmap (struct pt_cache *cache, size_t data_size, bool readonly)
{
    int prot = 0;
    void *addr;

    // determine prot
    prot |= PROT_READ;

    if (!readonly) {
        prot |= PROT_WRITE;
    }

    // mmap() the full file including header
    if ((addr = mmap(NULL, sizeof_pt_cache_file(data_size), prot, MAP_SHARED, cache->fd, 0)) == MAP_FAILED)
        return -PT_ERR_CACHE_MMAP;

    // ok
    cache->file = addr;
    cache->readonly = readonly;

    return 0;
}

int pt_cache_open (struct pt_cache *cache)
{
    PT_DEBUG("%s", cache->path);

    struct pt_cache_header header;
    int err;

    // ignore if already open
    if (cache->file)
        return 0;

    PT_DEBUG("%s", cache->path);

    // open the .cache in readonly mode
    if ((err = pt_open_cache_read_fd(cache->path, &cache->fd)))
        return err;

    // read in header
    if ((err = pt_cache_header_read(&header, cache->fd)))
        goto error;

    if ((err = pt_cache_header_check(&header)))
        goto error;

    // mmap the header + data
    if ((err = pt_cache_open_mmap(cache, header.data_size, true)))
        goto error;

    // done
    return 0;

error:
    // cleanup
    pt_cache_abort(cache);

    return err;
}

/**
 * Create a new .tmp cache file, open it, and write out the header.
 */
static int pt_cache_create (struct pt_cache *cache, struct pt_cache_header *header)
{
    int err;

    if (cache->file) {
      return -PT_ERR_CACHE_MODE;
    }

    PT_DEBUG("%s: data_size=%zu", cache->path, header->data_size);

    // open as .tmp
    if ((err = pt_cache_open_tmp_fd(cache, &cache->fd)))
        return err;

    // write header
    if ((err = pt_cache_header_write(header, cache->fd)))
        goto error;

    // grow file
    if (ftruncate(cache->fd, sizeof_pt_cache_file(header->data_size)) < 0) {
        err = -PT_ERR_CACHE_TRUNC;
        goto error;
      }

    // mmap header and data
    if ((err = pt_cache_open_mmap(cache, header->data_size, false)))
      goto error;

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
        return -PT_ERR_CACHE_RENAME_TMP;

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
        PT_WARN("pt_cache_tmp_name %s: %s", cache->path, pt_strerror(err));

        return;
    }

    // remove .tmp
    if (unlink(tmp_path))
        PT_WARN_ERRNO("unlink %s", tmp_path);
}

int pt_cache_update_png (struct pt_cache *cache, struct pt_png_img *img, const struct pt_image_params *params)
{
    struct pt_cache_header header = {
      .version = pt_cache_version,
      .magic   = PT_CACHE_MAGIC,
      .format  = PT_FORMAT_PNG,
    };
    int err;

    if (cache->file) {
      return -PT_ERR_CACHE_MODE;
    }

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

int pt_cache_render_tile (struct pt_cache *cache, struct pt_tile *tile)
{
    int err;

    if (!cache->file) {
      return -PT_ERR_CACHE_MODE;
    }

    // validate params
    if (!tile->params.width || !tile->params.height)
        return -PT_ERR_TILE_DIM;

    // render
    if ((err = pt_png_tile(&cache->file->header.png, cache->file->data, tile)))
        return err;

    return 0;
}

int pt_cache_close (struct pt_cache *cache)
{
    PT_DEBUG("%s", cache->path);

    if (cache->file != NULL) {
        if (munmap(cache->file, sizeof(struct pt_cache_file) + cache->file->header.data_size))
            return -PT_ERR_CACHE_MUNMAP;

        cache->file = NULL;
    }

    if (cache->fd >= 0) {
        if (close(cache->fd))
            return -PT_ERR_CACHE_CLOSE;

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
