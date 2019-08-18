#ifndef PNGTILE_CACHE_H
#define PNGTILE_CACHE_H

/**
 * @file
 *
 * Internal image cache implementation
 */
#include "png.h"

#include "pngtile.h"
#include <stdint.h>
#include <stdbool.h>

#define PT_CACHE_VERSION 5
#define PT_CACHE_MAGIC { 'P', 'N', 'G', 'T', 'I', 'L' }

/**
 * Size used to store the cache header
 */
#define PT_CACHE_HEADER_SIZE 4096

/**
 * On-disk header
 */
struct pt_cache_header {
    uint8_t magic[6];
    uint16_t version; // pt_cache_version

    /** Image format */
    enum pt_image_format format;

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
 * Check if given file is a cache file.
 *
 * @return <0 on error
 * @return 0 if valid cache file
 * @return 1 if not a valid cache file
 */
int pt_sniff_cache (const char *path);

/**
 * Verify if the cached data eixsts, or has become stale compared to the given original file.
 *
 * This will stat both the image file and cache file. This will also temporarily open the cache file to read the header version, if not already opened.
 *
 * @return one of pt_cache_status; <0 on error, 0 if fresh, >0 otherwise
 */
int pt_stat_cache (const char *path, const char *img_path);

/**
 * Get cached image info.
 *
 * Opens the cache temporarily if not already opened.
 *
 * @param cache_info optional
 * @param info returned info
 * @return from pt_cache_header()
 * @return -PT_ERR_CACHE_STAT
 */
 int pt_read_cache_info (const char *path, struct pt_cache_info *cache_info, struct pt_image_info *info);

/**
 * Cache state
 */
struct pt_cache {
    /** Filesystem path to cache file */
    char *path;

    /** Opened file */
    int fd;

    /** Size of the data segment in bytes, starting at PT_CACHE_HEADER_SIZE */
    size_t data_size;

    /** The mmap'd file */
    struct pt_cache_file *file;

    /** Opened read-only? */
    bool readonly;
};

/**
 * Construct the image cache info object associated with the given image.
 */
int pt_cache_new (struct pt_cache **cache_ptr, const char *path);

/**
 * initialize new cache file for PNG header.
 */
int pt_cache_create_png (struct pt_cache *cache, const struct pt_png_header *png_header, const struct pt_image_params *params);

/**
 * Update the cache data from the given PNG image data
 */
int pt_cache_update_png (struct pt_cache *cache, struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params);

/**
 * Update partial cache data from the given PNG image data
 */
int pt_cache_update_png_part (struct pt_cache *cache, struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params, unsigned row, unsigned col);

/**
 * Rename the opened .tmp to .cache
 */
int pt_cache_create_done (struct pt_cache *cache);

/**
 * Abort a failed cache update after cache_create
 */
void pt_cache_create_abort (struct pt_cache *cache);

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
