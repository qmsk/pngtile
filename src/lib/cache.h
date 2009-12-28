#ifndef PNGTILE_CACHE_H
#define PNGTILE_CACHE_H

/**
 * @file
 *
 * Internal image cache implementation
 */
#include "image.h"

#include <stdint.h>
#include <stdbool.h>

#include <png.h>

/**
 * State for cache access
 */
struct pt_cache {
    /** Filesystem path to cache file */
    char *path;

    /** The mode we are operating in, bitmask of PT_IMG_* */
    int mode;
    
    /** Opened file */
    int fd;

    /** Memory-mapped file data, from the second page on */
    uint8_t *mmap;

    /** Size of the mmap'd segment in bytes */
    size_t size;
};

/**
 * Size of a cache file page in bytes
 */
#define PT_CACHE_PAGE 4096

/**
 * On-disk header
 */
struct pt_cache_header {
    /** Pixel dimensions of image */
    uint32_t width, height;
    
    /** Pixel format */
    uint8_t bit_depth, color_type;

    /** Number of png_color entries that follow */
    uint16_t num_palette;

    /** Convenience field for number of bytes per row */
    uint32_t row_bytes;

    /** Palette entries, up to 256 entries used */
    png_color palette[PNG_MAX_PALETTE_LENGTH];
};

/**
 * Construct the image cache info object associated with the given image.
 */
int pt_cache_open (struct pt_cache **cache_ptr, const char *path, int mode);

/**
 * Verify if the cached data has become stale compared to the given original file.
 */
int pt_cache_stale (struct pt_cache *cache, const char *img_path);

/**
 * Update the cache data from the given PNG image.
 */
int pt_cache_update_png (struct pt_cache *cache, png_structp png, png_infop info);

/**
 * Release all resources associated with the given cache object without any cleanup.
 */
void pt_cache_destroy (struct pt_cache *cache);

#endif
