#ifndef PNGTILE_CACHE_H
#define PNGTILE_CACHE_H

/**
 * @file
 *
 * Internal image cache implementation
 */
#include "image.h"
#include "png.h"

#include <stdint.h>
#include <stdbool.h>

/**
 * Cache format version
 */
#define PT_CACHE_VERSION 3

/**
 * Size used to store the cache header
 */
#define PT_CACHE_HEADER_SIZE 4096

/**
 * On-disk header
 */
struct pt_cache_header {
    /** Set to PT_CACHE_VERSION */
    uint16_t version;

    /** Image format */
    enum pt_img_format {
        PT_IMG_PNG,     ///< @see pt_png
    } format;

    /** Data header by format  */
    union {
        struct pt_png_header png;
    };

    /** Parameters used */
    struct pt_image_params params;

    /** Size of the data segment */
    size_t data_size;
};

/**
 * On-disk data format. This struct is always exactly PT_CACHE_HEADER_SIZE long
 */
struct pt_cache_file {
    /** Header */
    struct pt_cache_header header;

    /** Padding for data */
    uint8_t padding[PT_CACHE_HEADER_SIZE - sizeof(struct pt_cache_header)];

    /** Data follows, header.data_size bytes */
    uint8_t data[];
};

/**
 * Cache state
 */
struct pt_cache {
    /** Filesystem path to cache file */
    char *path;

    /** The mode we are operating in, bitmask of PT_IMG_* */
    int mode;

    /** Opened file */
    int fd;

    /** Size of the data segment in bytes, starting at PT_CACHE_HEADER_SIZE */
    size_t data_size;

    /** The mmap'd file */
    struct pt_cache_file *file;
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
 * Get info for the cached image.
 *
 * Does not open it if not yet opened.
 */
void pt_cache_info (struct pt_cache *cache, struct pt_image_info *info);

/**
 * Update the cache data from the given image data
 */
int pt_cache_update (struct pt_cache *cache, struct pt_png_img *img, const struct pt_image_params *params);

/**
 * Open the existing .cache for use. If already opened, does nothing.
 */
int pt_cache_open (struct pt_cache *cache);

/**
 * Render out the given tile
 *
 * If the cache is not yet open, this will open it
 */
int pt_cache_render_tile (struct pt_cache *cache, struct pt_tile *tile);

/**
 * Close the cache, if opened
 */
int pt_cache_close (struct pt_cache *cache);

/**
 * Release all resources associated with the given cache object without any cleanup.
 */
void pt_cache_destroy (struct pt_cache *cache);

#endif
