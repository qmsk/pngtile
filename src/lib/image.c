#include "image.h"
#include "cache.h"

#include <stdlib.h>
#include <limits.h> // for _POSIX_PATH_MAX

#include <png.h>

static int pt_image_new (struct pt_image **img_ptr, struct pt_ctx *ctx, const char *path)
{
    struct pt_image *img;

    // alloc
    if ((img = calloc(1, sizeof(*img))) == NULL)
        return -1;

    if ((img->path = strdup(path)) == NULL)
        goto error;

    // init
    img->ctx = ctx;
    
    // ok
    *img_ptr = img;

    return 0;

error:
    pt_image_destroy(img);

    return -1;
}

/**
 * Open the image's FILE
 */
static int pt_image_open_file (struct pt_image *img, FILE **file_ptr)
{
    FILE *fp;
    
    // open
    if (fopen(img->path, "rb") < 0)
        return -1;

    // ok
    *file_ptr = fp;

    return 0;
}

/**
 * Open the PNG image, setting up the I/O and returning the png_structp and png_infop
 */
static int pt_image_open_png (struct pt_image *img, png_structp *png_ptr, png_infop *info_ptr)
{
    FILE *fp = NULL;
    png_structp png = NULL;
    png_infop info = NULL;
    
    // open I/O
    if (pt_image_open_file(img, &fp))
        goto error;

    // create the struct
    if ((png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
        goto error;

    // create the info
    if ((info = png_create_info_struct(png)) == NULL)
        goto error;

    // setup error trap for the I/O
    if (setjmp(png_jmpbuf(png)))
        goto error;
    
    // setup I/O to FILE
    png_init_io(png, fp);

    // ok
    // XXX: what to do with fp?
    *png_ptr = png;
    *info_ptr = info;

    return 0;

error:
    // cleanup file
    if (fp) fclose(fp);

    // cleanup PNG state
    png_destroy_read_struct(&png, &info, NULL);

    return -1;
}

/**
 * Open the PNG image, and write out to the cache
 */
static int pt_image_update_cache (struct pt_image *img)
{
    png_structp png;
    png_infop info;

    // open .png
    if (pt_image_open_png(img, &png, &info))
        return -1;
    
    // setup error trap
    if (setjmp(png_jmpbuf(png)))
        goto error;

    // read meta-info
    png_read_info(png, info);

    // pass to cache object
    if (pt_cache_update_png(img->cache, png, info))
        goto error;

    // finish off, ignore trailing data
    png_read_end(png, NULL);

    // clean up
    png_destroy_read_struct(&png, &info, NULL);

    return 0;

error:
    // clean up
    png_destroy_read_struct(&png, &info, NULL);

    return -1;
}

/**
 * Build a filesystem path representing the appropriate path for this image's cache entry, and store it in the given
 * buffer.
 */
static int pt_image_cache_path (struct pt_image *image, char *buf, size_t len)
{
    // TODO: impl
}

int pt_image_open (struct pt_image **image_ptr, struct pt_ctx *ctx, const char *path, int cache_mode)
{
    struct pt_image *image;
    char cache_path[_POSIX_PATH_MAX];

    // XXX: verify that the path exists and looks like a PNG file

    // alloc
    if (pt_image_new(&image, ctx, path))
        return -1;
    
    // compute cache file path
    if (pt_image_cache_path(image, cache_path, sizeof(cache_path)))
        goto error;

    // open the cache object for this image
    if (pt_cache_open(&image->cache, cache_path, cache_mode))
        goto error;
    
    // update if not fresh
    // XXX: check cache_mode
    // XXX: error handling
    if (pt_cache_stale(image->cache, image->path))
        pt_image_update_cache(image);
    
    // ok, ready for access
    *image_ptr = image;

    return 0;

error:
    pt_image_destroy(image);

    return -1;
}

void pt_image_destroy (struct pt_image *image)
{
    free(image->path);
    
    if (image->cache)
        pt_cache_destroy(image->cache);

    free(image);
}
