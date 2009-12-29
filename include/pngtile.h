#ifndef PNGTILE_H
#define PNGTILE_H

/**
 * @file
 *
 * Tile-based access to large PNG images.
 */
#include <stddef.h>
#include <stdio.h> // for FILE*

/**
 * "Global" context shared between images
 */
struct pt_ctx;

/**
 * Per-image state
 */
struct pt_image;

/** Bitmask for pt_image_open modes */
enum pt_open_mode {
    /** Update cache if needed */
    PT_OPEN_UPDATE   = 0x01,

    /** Accept stale cache */
    // TODO: PT_OPEN_STALE    = 0x02,
};

/**
 * Values for pt_image_cached
 */
enum pt_cache_status {
    /** Cache status could not be determined */
    PT_CACHE_ERROR      = -1,
    
    /** Cache is fresh */
    PT_CACHE_FRESH      = 0,

    /** Cache does not exist */
    PT_CACHE_NONE       = 1,

    /** Cache exists, but is stale */
    PT_CACHE_STALE      = 2,
};

/** Metadata info for image */
struct pt_image_info {
    /** Dimensions of image */
    size_t width, height;
};

/**
 * Info for image tile 
 *
 * The tile may safely overlap with the edge of the image, but it should not be entirely outside of the image
 */
struct pt_tile_info {
    /** Dimensions of output image */
    size_t width, height;

    /** Pixel coordinates of top-left corner */
    size_t x, y;

    /** Zoom factor (out < zero < in) */
    // TODO: int zoom;
};

/**
 * TODO: impl
 */
int pt_ctx_new (struct pt_ctx **ctx_ptr);

/**
 * Open a new pt_image for use.
 *
 * @param img_ptr returned pt_image handle
 * @param ctx global state to use
 * @param path filesystem path to .png file
 * @param mode combination of PT_OPEN_* flags
 */
int pt_image_open (struct pt_image **image_ptr, struct pt_ctx *ctx, const char *png_path, int cache_mode);

/**
 * Get the image's metadata
 */
int pt_image_info (struct pt_image *image, const struct pt_image_info **info_ptr);

/**
 * Check the given image's cache is stale - in other words, if the image needs to be update()'d.
 *
 * @return one of pt_cache_status
 */
int pt_image_status (struct pt_image *image);

/**
 * Update the given image's cache.
 */
int pt_image_update (struct pt_image *image);

/**
 * Render a PNG tile to a stream.
 *
 * The PNG data will be written to the given stream, which will be flushed, but not closed.
 */
int pt_image_tile (struct pt_image *image, const struct pt_tile_info *info, FILE *out);

/**
 * Release the given pt_image without any clean shutdown
 */
void pt_image_destroy (struct pt_image *image);

/**
 * Error codes returned
 */
enum pt_error {
    PT_SUCCESS = 0,
    PT_ERR_MEM,

    PT_ERR_PATH,
    PT_ERR_OPEN_MODE,
    
    PT_ERR_IMG_STAT,
    PT_ERR_IMG_FOPEN,
    
    PT_ERR_PNG_CREATE,
    PT_ERR_PNG,
   
    PT_ERR_CACHE_STAT,
    PT_ERR_CACHE_OPEN_READ,
    PT_ERR_CACHE_OPEN_TMP,
    PT_ERR_CACHE_SEEK,
    PT_ERR_CACHE_READ,
    PT_ERR_CACHE_WRITE,
    PT_ERR_CACHE_TRUNC,
    PT_ERR_CACHE_MMAP,
    PT_ERR_CACHE_RENAME_TMP,

    PT_ERR_TILE_CLIP,

    PT_ERR_MAX,
};

/**
 * Translate error code to short description
 */
const char *pt_strerror (int err);

#endif
