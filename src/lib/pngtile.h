#ifndef PNGTILE_H
#define PNGTILE_H

/**
 * @file
 *
 * Tile-based access to large PNG images.
 */
#include <stddef.h>

/**
 * "Global" context shared between images
 */
struct pt_ctx;

/**
 * Per-image state
 */
struct pt_image;

/** Bitmask for pt_image_open modes */
enum pt_image_mode {
    /** Update cache if needed */
    PT_IMG_WRITE    = 0x01,

    /** Accept stale cache */
    PT_IMG_STALE    = 0x02,
};

/** Metadata info for image */
struct pt_image_info {
    /** Dimensions of image */
    size_t width, height;
};


int pt_ctx_new (struct pt_ctx **ctx_ptr);

/**
 * Open a new pt_image for use.
 *
 * @param img_ptr returned pt_image handle
 * @param ctx global state to use
 * @param path filesystem path to .png file
 * @param mode combination of PT_IMG_* flags
 */
int pt_image_open (struct pt_image **image_ptr, struct pt_ctx *ctx, const char *png_path, int cache_mode);

/**
 * Get the image's metadata
 */
int pt_image_info (struct pt_image *image, const struct pt_image_info **info_ptr);

/**
 * Check the given image's cache is stale - in other words, the image needs to be updated.
 */
int pt_image_stale (struct pt_image *image);

/**
 * Update the given image's cache.
 */
int pt_image_update (struct pt_image *image);

/**
 * Release the given pt_image without any clean shutdown
 */
void pt_image_destroy (struct pt_image *image);

#endif
