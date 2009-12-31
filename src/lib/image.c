#include "image.h"
#include "ctx.h"
#include "cache.h"
#include "tile.h"
#include "error.h"
#include "shared/util.h"
#include "shared/log.h"

#include <stdlib.h>
#include <errno.h>

#include <png.h>

static int pt_image_new (struct pt_image **image_ptr, struct pt_ctx *ctx, const char *path)
{
    struct pt_image *image;
    int err = 0;

    // alloc
    if ((image = calloc(1, sizeof(*image))) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_MEM);

    if ((image->path = strdup(path)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_MEM);

    // init
    image->ctx = ctx;
    
    // ok
    *image_ptr = image;

    return 0;

error:
    pt_image_destroy(image);
    
    return err;
}

/**
 * Open the image's FILE
 */
static int pt_image_open_file (struct pt_image *image, FILE **file_ptr)
{
    FILE *fp;
    
    // open
    if ((fp = fopen(image->path, "rb")) == NULL)
        RETURN_ERROR(PT_ERR_IMG_FOPEN);

    // ok
    *file_ptr = fp;

    return 0;
}

/**
 * Open the PNG image, setting up the I/O and returning the png_structp and png_infop
 */
static int pt_image_open_png (struct pt_image *image, png_structp *png_ptr, png_infop *info_ptr)
{
    FILE *fp = NULL;
    png_structp png = NULL;
    png_infop info = NULL;
    int err;
    
    // open I/O
    if ((err = pt_image_open_file(image, &fp)))
        JUMP_ERROR(err);

    // create the struct
    if ((png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);

    // create the info
    if ((info = png_create_info_struct(png)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);

    // setup error trap for the I/O
    if (setjmp(png_jmpbuf(png)))
        JUMP_SET_ERROR(err, PT_ERR_PNG);
    
    // setup I/O to FILE
    png_init_io(png, fp);

    // ok
    // XXX: what to do with fp? Should fclose() when done?
    *png_ptr = png;
    *info_ptr = info;

    return 0;

error:
    // cleanup file
    if (fp) fclose(fp);

    // cleanup PNG state
    png_destroy_read_struct(&png, &info, NULL);
    
    return err;
}

/**
 * Update the image_info field from the given png object.
 *
 * Must be called under libpng-error-trap!
 *
 * XXX: currently this info is not used, pulled from the cache instead
 */
static int pt_image_update_info (struct pt_image *image, png_structp png, png_infop info)
{
    // query png_get_*
    image->info.width = png_get_image_width(png, info); 
    image->info.height = png_get_image_height(png, info); 

    return 0;
}

/**
 * Open the PNG image, and write out to the cache
 */
static int pt_image_update_cache (struct pt_image *image)
{
    png_structp png;
    png_infop info;
    int err = 0;

    // pre-check enabled
    if (!(image->cache->mode & PT_OPEN_UPDATE))
        RETURN_ERROR_ERRNO(PT_ERR_OPEN_MODE, EACCES);

    // open .png
    if ((err = pt_image_open_png(image, &png, &info)))
        return err;
    
    // setup error trap
    if (setjmp(png_jmpbuf(png)))
        JUMP_SET_ERROR(err, PT_ERR_PNG);

    // read meta-info
    png_read_info(png, info);

    // update our meta-info
    if ((err = pt_image_update_info(image, png, info)))
        JUMP_ERROR(err);

    // pass to cache object
    if ((err = pt_cache_update_png(image->cache, png, info)))
        JUMP_ERROR(err);

    // finish off, ignore trailing data
    png_read_end(png, NULL);

error:
    // clean up
    // XXX: we need to close the fopen'd .png
    png_destroy_read_struct(&png, &info, NULL);

    return err;
}

/**
 * Build a filesystem path representing the appropriate path for this image's cache entry, and store it in the given
 * buffer.
 */
static int pt_image_cache_path (struct pt_image *image, char *buf, size_t len)
{
    if (path_with_fext(image->path, buf, len, ".cache"))
        RETURN_ERROR(PT_ERR_PATH);

    return 0;
}

int pt_image_open (struct pt_image **image_ptr, struct pt_ctx *ctx, const char *path, int cache_mode)
{
    struct pt_image *image;
    char cache_path[1024];
    int err;

    // XXX: verify that the path exists and looks like a PNG file

    // alloc
    if ((err = pt_image_new(&image, ctx, path)))
        return err;
    
    // compute cache file path
    if ((err = pt_image_cache_path(image, cache_path, sizeof(cache_path))))
        JUMP_ERROR(err);

    // create the cache object for this image (doesn't yet open it)
    if ((err = pt_cache_new(&image->cache, cache_path, cache_mode)))
        JUMP_ERROR(err);
    
    // ok, ready for access
    *image_ptr = image;

    return 0;

error:
    pt_image_destroy(image);

    return err;
}

int pt_image_info (struct pt_image *image, const struct pt_image_info **info_ptr)
{
    int err;

    // update info
    if ((err = pt_cache_info(image->cache, &image->info)))
        return err;
    
    // return pointer
    *info_ptr = &image->info;

    return 0;
}

int pt_image_status (struct pt_image *image)
{
    return pt_cache_status(image->cache, image->path);
}

int pt_image_update (struct pt_image *image)
{
    return pt_image_update_cache(image);
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
        JUMP_ERROR(err);

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
        JUMP_ERROR(err);

    // ok
    *buf_ptr = tile.out.mem.base;
    *len_ptr = tile.out.mem.len;

    return 0;

error:
    pt_tile_abort(&tile);

    return err;
}

static void _pt_image_tile_async (void *arg)
{
    struct pt_tile *tile = arg;
    int err;

    // do render op
    if ((err = pt_tile_render(tile)))
        log_warn_errno("pt_tile_render: %s", pt_strerror(err));

    // signal done
    if (fclose(tile->out.file))
        log_warn_errno("fclose");

    // cleanup
    pt_tile_destroy(tile);
}

int pt_image_tile_async (struct pt_image *image, const struct pt_tile_info *info, FILE *out)
{
    struct pt_tile *tile;
    int err;

    // alloc
    if ((err = pt_tile_new(&tile)))
        return err;

    // init
    if ((err = pt_tile_init_file(tile, image->cache, info, out)))
        JUMP_ERROR(err);
    
    // enqueue work
    if ((err = pt_ctx_work(image->ctx, _pt_image_tile_async, tile)))
        JUMP_ERROR(err);

    // ok, running
    return 0;

error:
    pt_tile_destroy(tile);

    return err;
}

void pt_image_destroy (struct pt_image *image)
{
    free(image->path);
    
    if (image->cache)
        pt_cache_destroy(image->cache);

    free(image);
}

