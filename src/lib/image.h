#ifndef PNGTILE_IMAGE_H
#define PNGTILE_IMAGE_H

/**
 * @file
 *
 * Internal pt_image state
 */
#include "pngtile.h"

struct pt_image {
    enum pt_image_format format;
    
    /** Path to .png */
    char *path;

    /** Cache object */
    struct pt_cache *cache;
};

#endif
