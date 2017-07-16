#include "image.h"
#include "png.h"
#include "cache.h"
#include "tile.h"
#include "path.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static int pt_image_alloc (struct pt_image **image_ptr, const char *path, enum pt_image_format format)
{
    struct pt_image *image;
    int err = 0;

    // alloc
    if ((image = calloc(1, sizeof(*image))) == NULL) {
        return -PT_ERR_MEM;
    }

    image->format = format;

    if ((image->path = strdup(path)) == NULL) {
        err = -PT_ERR_MEM;
        goto error;
    }

    // ok
    *image_ptr = image;

    return 0;

error:
    pt_image_destroy(image);

    return err;
}

/**
 * Build a filesystem path representing the appropriate path for this image's cache entry, and store it in the given
 * buffer.
 *
 * If this is a PT_IMAGE_CACHE, then this is going to be identical to ->path..
 */
static int pt_image_cache_path (struct pt_image *image, char *buf, size_t len)
{
    if (pt_path_make_ext(buf, len, image->path, ".cache")) {
        return -PT_ERR_PATH;
    }

    return 0;
}

int pt_image_sniff (const char *path, enum pt_image_format *format)
{
  const char *ext = pt_path_ext(path);

  PT_DEBUG("%s: ext=%s", path, ext);

  if (strcmp(ext, ".cache") == 0) {
    *format = PT_FORMAT_CACHE;

    return pt_cache_check(path);

  } else if (strcmp(ext, ".png") == 0) {
    *format = PT_FORMAT_PNG;

    return pt_png_check(path);

  } else {
    return 1;
  }
}

int pt_image_new (struct pt_image **image_ptr, const char *path, int cache_mode)
{
    PT_DEBUG("%s: cache_mode=%d", path, cache_mode);

    struct pt_image *image;
    char cache_path[1024];
    enum pt_image_format format;
    int err;

    // verify that the path exists and looks like a valid file
    if ((err = pt_image_sniff(path, &format)))
        return err > 0 ? -PT_ERR_IMG_FORMAT : err;

    // alloc
    if ((err = pt_image_alloc(&image, path, format)))
        return err;

    // compute cache file path
    if ((err = pt_image_cache_path(image, cache_path, sizeof(cache_path))))
        goto error;

    // create the cache object for this image (doesn't yet open it)
    if ((err = pt_cache_new(&image->cache, cache_path, cache_mode)))
        goto error;

    // ok, ready for access
    *image_ptr = image;

    return 0;

error:
    pt_image_destroy(image);

    return err;
}

int pt_image_status (struct pt_image *image)
{
    return pt_cache_status(image->cache, image->path);
}

// Open PNG file
static int pt_image_open_file (struct pt_image *image, FILE **file_ptr)
{
    FILE *fp;

    // open
    if ((fp = fopen(image->path, "rb")) == NULL)
        return -PT_ERR_IMG_OPEN;

    // ok
    *file_ptr = fp;

    return 0;
}

/**
 * Open the PNG image, and write out to the cache
 */
int pt_image_update_png (struct pt_image *image, const struct pt_image_params *params)
{
    PT_DEBUG("%s: params=%p", image->path, params);

    FILE *file;
    struct pt_png_img png_img;
    int err = 0;

    // open .png file
    if ((err = pt_image_open_file(image, &file)))
        return err;

    if ((err = pt_png_open(&png_img, file)))
        return err;

    // pass to cache object
    if ((err = pt_cache_update_png(image->cache, &png_img, params)))
        goto error;

error:
    // clean up
    pt_png_release_read(&png_img);

    return err;
}

/**
 * Open the PNG image, and write out to the cache
 */
int pt_image_update (struct pt_image *image, const struct pt_image_params *params)
{
    PT_DEBUG("%s: params=%p", image->path, params);

    // pre-check enabled
    if (!(image->cache->mode & PT_OPEN_UPDATE))
        return -PT_ERR_OPEN_MODE;

    switch (image->format) {
      case PT_FORMAT_CACHE:
        // can't update a .cache image
        return -PT_ERR_IMG_FORMAT_CACHE;

      case PT_FORMAT_PNG:
        return pt_image_update_png(image, params);

      default:
        return -PT_ERR_IMG_FORMAT;
    }
}

int pt_image_info (struct pt_image *image, struct pt_image_info *info)
{
    struct stat st;
    int err;

    info->image_format = image->format;

    if (stat(image->path, &st) < 0) {
        return -PT_ERR_IMG_STAT;
    }

    // image file info
    info->image_mtime = st.st_mtime;
    info->image_bytes = st.st_size;

    // update info from cache
    if ((err = pt_cache_info(image->cache, info))) {
      return err;
    }

    PT_DEBUG("%s: image width=%d height=%d", image->path, info->image_width, info->image_height);

    return 0;
}

int pt_image_open (struct pt_image *image)
{
    PT_DEBUG("%s", image->path);

    return pt_cache_open(image->cache);
}

int pt_image_tile_file (struct pt_image *image, const struct pt_tile_params *params, FILE *out)
{
    PT_DEBUG("%s: width=%u height=%u x=%u y=%u zoom=%d", image->path, params->width, params->height, params->x, params->y, params->zoom);

    struct pt_tile tile;
    int err;

    // init
    if ((err = pt_tile_init_file(&tile, params, out)))
        return err;

    // render
    if ((err = pt_cache_render_tile(image->cache, &tile)))
        goto error;

    // ok
    return 0;

error:
    pt_tile_abort(&tile);

    return err;
}

int pt_image_tile_mem (struct pt_image *image, const struct pt_tile_params *params, char **buf_ptr, size_t *len_ptr)
{
    PT_DEBUG("%s: width=%u height=%u x=%u y=%u zoom=%d", image->path, params->width, params->height, params->x, params->y, params->zoom);

    struct pt_tile tile;
    int err;

    // init
    if ((err = pt_tile_init_mem(&tile,  params)))
        return err;

    // render
    if ((err = pt_cache_render_tile(image->cache, &tile)))
        goto error;

    // ok
    *buf_ptr = tile.out.mem.base;
    *len_ptr = tile.out.mem.len; // XXX: should be .off

    return 0;

error:
    pt_tile_abort(&tile);

    return err;
}

int pt_image_close (struct pt_image *image)
{
  PT_DEBUG("%s", image->path);

  int err;

  if (image->cache && (err = pt_cache_close(image->cache)))
    return err;

  return 0;
}

void pt_image_destroy (struct pt_image *image)
{
    PT_DEBUG("%s", image->path);

    free(image->path);

    if (image->cache)
        pt_cache_destroy(image->cache);

    free(image);
}
