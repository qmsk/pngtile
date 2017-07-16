#ifndef PNGTILE_H
#define PNGTILE_H

/**
 * @file
 *
 * Tile-based access to large PNG images.
 */
#include <stddef.h>
#include <stdio.h> // for FILE*
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h> // for time_t

/**
 * Per-image state
 */
struct pt_image;

/** Bitmask for pt_image_open modes */
enum pt_open_mode {
    /** Open cache for read*/
    PT_OPEN_READ    = 0x00,

    /** Open cache for update */
    PT_OPEN_UPDATE   = 0x01,

    /** Accept stale cache */
    // TODO: PT_OPEN_STALE    = 0x02,
};

/**
 * Values for pt_image_cached
 */
enum pt_cache_status {
    /** Cache is fresh */
    PT_CACHE_FRESH      = 0,

    /** Cache does not exist */
    PT_CACHE_NONE       = 1,

    /** Cache exists, but is stale */
    PT_CACHE_STALE      = 2,

    /** Cache exists, but it was generated using an incompatible version of this library */
    PT_CACHE_INCOMPAT   = 3,
};

/**
 * Image formats.
 */
enum pt_image_format {
    PT_FORMAT_CACHE    = 0, // no source image, opened cache directly
    PT_FORMAT_PNG,
};

/** Metadata info for image. Values will be set to zero if not available */
struct pt_image_info {
    enum pt_image_format image_format;

    /** Dimensions of image. Only available if the cache is open */
    size_t image_width, image_height;

    /** Bits per pixel */
    size_t image_bpp;

    /** Last update of image file */
    time_t image_mtime;

    /** Size of image file in bytes */
    size_t image_bytes;

    /** Cache format version or -err */
    int cache_version;

    enum pt_image_format cache_format;

    /** Last update of cache file */
    time_t cache_mtime;

    /** Size of cache file in bytes */
    size_t cache_bytes;

    /** Size of cache file in blocks (for sparse cache files) - 512 bytes / block? */
    size_t cache_blocks;
};

/**
 * Sparse background color pattern: matched aginst each individual PNG pixel in each block of pt_image_block_size pixels.
 * Must be in the same raw format that the PNG data is in.
 *
 * Blocks containing only background pixels are not written to the cache file, allowing the use of sparse files.
 */
typedef uint8_t pt_image_pixel[4];

/**
 * Handle sparse data at this granularity (pixels)
 */
extern const size_t pt_image_block_size; // 64

/**
 * Modifyable params for update
 */
struct pt_image_params {
    enum {
      PT_IMAGE_BACKGROUND_PIXEL = 1,
    } flags;

    /** Don't write out any contiguous regions of this color. Left-aligned in whatever format the source image is in */
    pt_image_pixel background_pixel;
};

/**
 * Parameters for tile render.
 *
 * The tile may safely overlap with the edge of the image, but it should not be entirely outside of the image
 */
struct pt_tile_params {
    /** Dimensions of output image */
    unsigned int width, height;

    /** Pixel coordinates of top-left corner */
    unsigned int x, y;

    /** Zoom factor of 2^z (out < zero < in) */
    int zoom;
};

/**
 * Enable/Disable DEBUG logging to stderr.
 */
extern bool pt_log_debug;

/**
 * Enable/Disable WARN logging to stderr.
 *
 * Warnings are logged on errors that happen during error recovery.
 */
extern bool pt_log_warn;

/**
 * Test if the given file looks like something that you can open.
 *
 * No point calling this before pt_image_new, which already calls this.
 *
 * @param format return detected format if 0
 * @return 0 if ok
 * @return 1 if unknown
 */
int pt_image_sniff (const char *path, enum pt_image_format *format);

/**
 * Open a new pt_image for use.
 *
 * @param img_ptr returned pt_image handle * @param path filesystem path to .png file
 * @param mode combination of PT_OPEN_* flags
 * @return -PT_ERR_FORMAT if pt_image_sniff fails
 */
int pt_image_new (struct pt_image **image_ptr, const char *png_path, int cache_mode);

/**
 * Get the image's metadata.
 *
 * Opens the cache temporarily if not already open.
 *
 */
int pt_image_info (struct pt_image *image, struct pt_image_info *info_ptr);

/**
 * Check the given image's cache is stale - in other words, if the image needs to be update()'d.
 *
 * @return pt_cache_status, < 0 on error.
 */
int pt_image_status (struct pt_image *image);

/**
 * Update the given image's cache.
 *
 * Fails if not opened with PT_OPEN_UPDATE.
 *
 * @param params optional parameters to use for the update process
 */
int pt_image_update (struct pt_image *image, const struct pt_image_params *params);

/**
 * Load the image's cache in read-only mode without trying to update it.
 *
 * Fails if the cache doesn't exist.
 */
int pt_image_open (struct pt_image *image);

/**
 * Render a PNG tile to a FILE*.
 *
 * The PNG data will be written to the given stream, which will be flushed, but not closed.
 *
 * Tile render operations are threadsafe as long as the pt_image is not modified during execution: call pt_image_load() first.
 */
int pt_image_tile_file (struct pt_image *image, const struct pt_tile_params *params, FILE *out);

/**
 * Render a PNG tile to memory.
 *
 * The PNG data will be written to a malloc'd buffer.
 *
 * Tile render operations are threadsafe as long as the pt_image is not modified during execution: call pt_image_load() first.
 *
 * @param image render from image's cache
 * @param params tile parameters
 * @param buf_ptr returned heap buffer
 * @param len_ptr returned buffer length
 */
int pt_image_tile_mem (struct pt_image *image, const struct pt_tile_params *params, char **buf_ptr, size_t *len_ptr);

/**
 * Close associated resources, returning error.
 *
 * The pt_image remains valid, and can be re-used after pt_image_open().
 *
 * Must still call pt_image_destroy() afterwards.
 */
int pt_image_close (struct pt_image *image);

/**
 * Release the given pt_image. If it is still open, it will be aborted, warning on close errors.
 */
void pt_image_destroy (struct pt_image *image);

/**
 * Error codes returned
 */
enum pt_error {
    /** No error */
    PT_SUCCESS = 0,

    /** Generic error */
    PT_ERR = 1,

    PT_ERR_MEM,

    PT_ERR_PATH,
    PT_ERR_OPEN_MODE,

    PT_ERR_IMG_STAT,
    PT_ERR_IMG_OPEN,
    PT_ERR_IMG_FORMAT,
    PT_ERR_IMG_FORMAT_INTERLACE,
    PT_ERR_IMG_FORMAT_CACHE,

    PT_ERR_PNG_CREATE,
    PT_ERR_PNG,

    PT_ERR_CACHE_STAT,
    PT_ERR_CACHE_OPEN_READ,
    PT_ERR_CACHE_OPEN_TMP,
    PT_ERR_CACHE_SEEK,
    PT_ERR_CACHE_READ,
    PT_ERR_CACHE_WRITE,
    PT_ERR_CACHE_TRUNC,
    PT_ERR_CACHE_MMAP,
    PT_ERR_CACHE_RENAME_TMP,
    PT_ERR_CACHE_MAGIC,
    PT_ERR_CACHE_VERSION,
    PT_ERR_CACHE_FORMAT,
    PT_ERR_CACHE_MUNMAP,
    PT_ERR_CACHE_CLOSE,

    PT_ERR_TILE_DIM,
    PT_ERR_TILE_CLIP,
    PT_ERR_TILE_ZOOM,


    PT_ERR_MAX,
};

/**
 * Translate error code to short description
 */
const char *pt_strerror (int err);

#endif
