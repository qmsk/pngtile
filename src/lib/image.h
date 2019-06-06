#ifndef PNGTILE_IMAGE_H
#define PNGTILE_IMAGE_H

/**
 * @file
 *
 * Internal pt_image state
 */
#include "pngtile.h"

struct pt_image {
    /** Path to cache file */
    char *cache_path;

    /** Cache file */
    struct pt_cache *cache;
};

#endif
