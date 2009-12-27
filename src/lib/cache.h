#ifndef PNGTILE_CACHE_H
#define PNGTILE_CACHE_H

/**
 * @file
 *
 * Internal image cache implementation
 */
#include "image.h"

/**
 * State for cache access
 */
struct pt_cache {

};

/**
 * On-disk header
 */
struct pt_cache_header {
    /** Pixel dimensions of image */
    uint32_t width, height;
    
    /** Pixel format */
    uint8_t bit_depth, color_type;

    /** Convenience field for number of bytes per row */
    uint32_t row_bytes;
};

/**
 * Construct the image cache info object associated with the given image.
 */
int pt_cache_open (struct pt_cache **cache_ptr, struct pt_image *img, int mode);

/**
 * Verify if the cached data is still fresh compared to the original.
 */
bool pt_cache_fresh (struct pt_cache *cache);

/**
 * Update the cache data from the given PNG image.
 */
int pt_cache_update_png (struct pt_cache *cache, png_structp png, png_infop info);

#endif
