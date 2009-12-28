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

    /** The mmap'd header */
    struct pt_cache_header *header;

    /** Memory-mapped file data, starting at PT_CACHE_HEADER_SIZE */
    uint8_t *data;

    /** Size of the data segment in bytes, starting at PT_CACHE_HEADER_SIZE */
    size_t size;
};

/**
 * Size used to store the cache header
 */
#define PT_CACHE_HEADER_SIZE 4096

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
    
    /** Number of bytes per pixel */
    uint8_t col_bytes;

    /** Palette entries, up to 256 entries used */
    png_color palette[PNG_MAX_PALETTE_LENGTH];
};

/**
 * Construct the image cache info object associated with the given image.
 */
int pt_cache_new (struct pt_cache **cache_ptr, const char *path, int mode);

/**
 * Verify if the cached data eixsts, or has become stale compared to the given original file.
 *
 * @return one of pt_cache_status; <0 on error, 0 if fresh, >0 otherwise
 */
int pt_cache_status (struct pt_cache *cache, const char *img_path);

/**
 * Update the cache data from the given PNG image.
 */
int pt_cache_update_png (struct pt_cache *cache, png_structp png, png_infop info);

/**
 * Actually open the existing .cache for use
 */
int pt_cache_open (struct pt_cache *cache);

/**
 * Render out a PNG tile as given, into the established png object, up to (but not including) the png_write_end.
 *
 * If the cache is not yet open, this will open it
 */
int pt_cache_tile_png (struct pt_cache *cache, png_structp png, png_infop info, const struct pt_tile_info *ti);

/**
 * Release all resources associated with the given cache object without any cleanup.
 */
void pt_cache_destroy (struct pt_cache *cache);

#endif
