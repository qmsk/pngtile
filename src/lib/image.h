#ifndef PNGTILE_IMAGE_H
#define PNGTILE_IMAGE_H

/**
 * @file
 *
 * Internal pt_image state
 */
#include "pngtile.h"

struct pt_image {
    /** Associated global state */
    struct pt_ctx *ctx;

    /** Path to .png */
    char *path;
    
    /** Cache object */
    struct pt_cache *cache;
};

/**
 * Release the given pt_image without any clean shutdown
 */
void pt_image_destroy (struct pt_image *image);


#endif
