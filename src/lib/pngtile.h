#ifndef PNGTILE_H
#define PNGTILE_H

/**
 * @file
 *
 * Tile-based access to large PNG images.
 */

/**
 * "Global" context shared between images
 */
struct pt_ctx;

/**
 * Per-image state
 */
struct pt_image;

enum pt_image_mode {
    PT_IMG_READ     = 0x01,
    PT_IMG_WRITE    = 0x02,
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
 * Check the given image's cache is stale - in other words, the image needs to be updated.
 */
int pt_image_stale (struct pt_image *image);

/**
 * Update the given image's cache.
 */
int pt_image_update (struct pt_image *image);

#endif
