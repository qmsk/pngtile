#include "image.h"
#include "png.h"
#include "cache.h"
#include "tile.h"
#include "shared/util.h"
#include "shared/log.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static int pt_image_new (struct pt_image **image_ptr, const char *path)
{
    struct pt_image *image;
    int err = 0;

    // alloc
    if ((image = calloc(1, sizeof(*image))) == NULL) {
        return -PT_ERR_MEM;
    }

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
 */
static int pt_image_cache_path (struct pt_image *image, char *buf, size_t len)
{
    if (path_with_fext(image->path, buf, len, ".cache")) {
        return -PT_ERR_PATH;
    }

    return 0;
}

int pt_image_open (struct pt_image **image_ptr, const char *path, int cache_mode)
{
    struct pt_image *image;
    char cache_path[1024];
    int err;

    // verify that the path exists and looks like a PNG file
    if ((err = pt_png_check(path)) < 0)
        return err;

    if (err)
      // fail, not a PNG
        return -PT_ERR_IMG_FORMAT;

    // alloc
    if ((err = pt_image_new(&image, path)))
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

int pt_image_open_file (struct pt_image *image, FILE **file_ptr)
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
int pt_image_update (struct pt_image *image, const struct pt_image_params *params)
{
    struct pt_png_img img;
    int err = 0;

    // pre-check enabled
    if (!(image->cache->mode & PT_OPEN_UPDATE))
        return -PT_ERR_OPEN_MODE;

    // open .png
    if ((err = pt_png_open(image, &img)))
        return err;

    // pass to cache object
    if ((err = pt_cache_update(image->cache, &img, params)))
        goto error;

error:
    // clean up
    pt_png_release_read(&img);

    return err;
}

int pt_image_info (struct pt_image *image, const struct pt_image_info **info_ptr)
{
    struct stat st;

    // update info from cache
    pt_cache_info(image->cache, &image->info);

    // stat our info
    if (stat(image->path, &st) < 0) {
        // unknown
        image->info.image_mtime = 0;
        image->info.image_bytes = 0;

    } else {
        // store
        image->info.image_mtime = st.st_mtime;
        image->info.image_bytes = st.st_size;
    }

    // return pointer
    *info_ptr = &image->info;

    return 0;
}

int pt_image_status (struct pt_image *image)
{
    return pt_cache_status(image->cache, image->path);
}


int pt_image_load (struct pt_image *image)
{
    return pt_cache_open(image->cache);
}

int pt_image_tile_file (struct pt_image *image, const struct pt_tile_info *info, FILE *out)
{
    struct pt_tile tile;
    int err;

    // init
    if ((err = pt_tile_init_file(&tile, image->cache, info, out)))
        return err;

    // render
    if ((err = pt_tile_render(&tile)))
        goto error;

    // ok
    return 0;

error:
    pt_tile_abort(&tile);

    return err;
}

int pt_image_tile_mem (struct pt_image *image, const struct pt_tile_info *info, char **buf_ptr, size_t *len_ptr)
{
    struct pt_tile tile;
    int err;

    // init
    if ((err = pt_tile_init_mem(&tile, image->cache, info)))
        return err;

    // render
    if ((err = pt_tile_render(&tile)))
        goto error;

    // ok
    *buf_ptr = tile.out.mem.base;
    *len_ptr = tile.out.mem.len;

    return 0;

error:
    pt_tile_abort(&tile);

    return err;
}

void pt_image_destroy (struct pt_image *image)
{
    free(image->path);

    if (image->cache)
        pt_cache_destroy(image->cache);

    free(image);
}
