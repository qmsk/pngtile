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

int pt_sniff_image (const char *path, enum pt_image_format *format)
{
  const char *ext = pt_path_ext(path);

  PT_DEBUG("%s: ext=%s", path, ext);

  if (strcmp(ext, ".cache") == 0) {
    *format = PT_FORMAT_CACHE;

    return pt_sniff_cache(path);

  } else if (strcmp(ext, ".png") == 0) {
    *format = PT_FORMAT_PNG;

    return pt_sniff_png(path);

  } else {
    return 1;
  }
}

int pt_read_image_info (const char *path, struct pt_image_info *info)
{
    int err;

    // verify that the path exists and looks like a valid file
    if ((err = pt_sniff_image(path, &info->format)))
        return err > 0 ? -PT_ERR_IMG_FORMAT : err;

/*
    if (stat(path, &st) < 0) {
        return -PT_ERR_IMG_STAT;
    }

    // image file info
    info->mtime = st.st_mtime;
    info->bytes = st.st_size;

    PT_DEBUG("%s: path=%s image bytes=%zu", image->cache_path, path, info->bytes);
*/

    switch (info->format) {
      case PT_FORMAT_CACHE:
        return pt_read_cache_info(path, NULL, info);

      case PT_FORMAT_PNG:
        return pt_read_png_info(path, info);
    }

    return 0;
}


static int pt_image_alloc (struct pt_image **image_ptr, const char *cache_path)
{
    struct pt_image *image;
    int err = 0;

    // alloc
    if ((image = calloc(1, sizeof(*image))) == NULL) {
        return -PT_ERR_MEM;
    }

    if ((image->cache_path = strdup(cache_path)) == NULL) {
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

int pt_image_new (struct pt_image **image_ptr, const char *cache_path)
{
    PT_DEBUG("%s", cache_path);

    struct pt_image *image;
    int err;

    // alloc
    if ((err = pt_image_alloc(&image, cache_path)))
        return err;

    // ok, ready for access
    *image_ptr = image;

    return 0;
}

int pt_image_status (struct pt_image *image, const char *path)
{
    PT_DEBUG("%s: path=%s", image->cache_path, path);

    return pt_stat_cache(image->cache_path, path);
}

int pt_image_info (struct pt_image *image, struct pt_cache_info *cache_info, struct pt_image_info *info)
{
    int err;

    // update info from cache
    if ((err = pt_read_cache_info(image->cache_path, cache_info, info))) {
      return err;
    }

    PT_DEBUG("%s: cache version=%d width=%d height=%d", image->cache_path, cache_info->version, info->width, info->height);

    return 0;
}

/**
 * Open the PNG image, and write out to the cache
 */
int pt_image_update_png (struct pt_image *image, const char *path, const struct pt_image_params *params)
{
    PT_DEBUG("%s: path=%s params=%p", image->cache_path, path, params);

    struct pt_png_img png_img;
    int err = 0;

    if ((err = pt_png_open_path(&png_img, path)))
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
int pt_image_update (struct pt_image *image, const char *path, const struct pt_image_params *params)
{
    enum pt_image_format format;
    int err;

    if (image->cache)
      return -PT_ERR_IMG_MODE;

    PT_DEBUG("%s: path=%s params=%p", image->cache_path, path, params);

    // verify that the path exists and looks like a valid file
    if ((err = pt_sniff_image(path, &format)))
        return err > 0 ? -PT_ERR_IMG_FORMAT : err;

    // create the cache object for this image (doesn't yet open it)
    if ((err = pt_cache_new(&image->cache, image->cache_path)))
        return err;

    switch (format) {
      case PT_FORMAT_CACHE:
        // can't update a .cache image
        return -PT_ERR_IMG_FORMAT_CACHE;

      case PT_FORMAT_PNG:
        return pt_image_update_png(image, path, params);

      default:
        return -PT_ERR_IMG_FORMAT;
    }
}

int pt_image_open (struct pt_image *image)
{
    int err;

    if (image->cache)
      return -PT_ERR_IMG_MODE;

    PT_DEBUG("%s", image->cache_path);

    // create the cache object for this image (doesn't yet open it)
    if ((err = pt_cache_new(&image->cache, image->cache_path)))
        return err;

    return pt_cache_open(image->cache);
}

int pt_image_tile_file (struct pt_image *image, const struct pt_tile_params *params, FILE *out)
{
    if (!image->cache)
      return -PT_ERR_IMG_MODE;

    PT_DEBUG("%s: width=%u height=%u x=%u y=%u zoom=%d", image->cache_path, params->width, params->height, params->x, params->y, params->zoom);

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
    if (!image->cache)
      return -PT_ERR_IMG_MODE;

    PT_DEBUG("%s: width=%u height=%u x=%u y=%u zoom=%d", image->cache_path, params->width, params->height, params->x, params->y, params->zoom);

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
  PT_DEBUG("%s", image->cache_path);

  int err;

  if (image->cache && (err = pt_cache_close(image->cache)))
    return err;

  if (image->cache) {
    pt_cache_destroy(image->cache);

    image->cache = NULL;
  }

  return 0;
}

void pt_image_destroy (struct pt_image *image)
{
    PT_DEBUG("%s", image->cache_path);

    if (image->cache)
        pt_cache_destroy(image->cache);

    free(image->cache_path);
    free(image);
}
