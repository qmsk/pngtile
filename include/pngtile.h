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
#include <sys/types.h> // for time_t

/**
 * "Global" context shared between images
 */
struct pt_ctx;

/**
 * Per-image state
 */
struct pt_image;

/** Bitmask for pt_image_open modes */
enum pt_open_mode {
    /** Update cache if needed */
    PT_OPEN_UPDATE   = 0x01,

    /** Accept stale cache */
    // TODO: PT_OPEN_STALE    = 0x02,
};

/**
 * Values for pt_image_cached
 */
enum pt_cache_status {
    /** Cache status could not be determined */
    PT_CACHE_ERROR      = -1,
    
    /** Cache is fresh */
    PT_CACHE_FRESH      = 0,

    /** Cache does not exist */
    PT_CACHE_NONE       = 1,

    /** Cache exists, but is stale */
    PT_CACHE_STALE      = 2,

    /** Cache exists, but it was generated using an incompatible version of this library */
    PT_CACHE_INCOMPAT   = 3,
};

/** Metadata info for image. Values will be set to zero if not available */
struct pt_image_info {
    /** Dimensions of image. Only available if the cache is open */
    size_t img_width, img_height;

    /** Bits per pixel */
    size_t img_bpp;

    /** Last update of image file */
    time_t image_mtime;

    /** Size of image file in bytes */
    size_t image_bytes;
    
    /** Cache format version or -err */
    int cache_version;

    /** Last update of cache file */
    time_t cache_mtime;

    /** Size of cache file in bytes */
    size_t cache_bytes;

    /** Size of cache file in blocks (for sparse cache files) - 512 bytes / block? */
    size_t cache_blocks;
};

/**
 * Modifyable params for update
 */
struct pt_image_params {
    /** Don't write out any contiguous regions of this color. Left-aligned in whatever format the source image is in */
    uint8_t background_color[4];
};

/**
 * Info for image tile 
 *
 * The tile may safely overlap with the edge of the image, but it should not be entirely outside of the image
 */
struct pt_tile_info {
    /** Dimensions of output image */
    size_t width, height;

    /** Pixel coordinates of top-left corner */
    size_t x, y;

    /** Zoom factor of 2^z (out < zero < in) */
    int zoom;
};

/**
 * Construct a new pt_ctx for use with further pt_image's.
 *
 * @param ctx_ptr returned pt_ctx handle
 * @param threads number of worker threads to use for parralel operations, or zero to disable
 */
int pt_ctx_new (struct pt_ctx **ctx_ptr, int threads);

/**
 * Shut down the given pt_ctx, waiting for any ongoing/pending operations to finish.
 */
int pt_ctx_shutdown (struct pt_ctx *ctx);

/**
 * Release the given pt_ctx without waiting for any ongoing operations to finish.
 */
void pt_ctx_destroy (struct pt_ctx *ctx);

/**
 * Open a new pt_image for use.
 *
 * The pt_ctx is optional, but required for pt_image_tile_async.
 *
 * @param img_ptr returned pt_image handle
 * @param ctx global state to use (optional)
 * @param path filesystem path to .png file
 * @param mode combination of PT_OPEN_* flags
 */
int pt_image_open (struct pt_image **image_ptr, struct pt_ctx *ctx, const char *png_path, int cache_mode);

/**
 * Get the image's metadata.
 *
 * XXX: return void, this never fails, just returns partial info
 */
int pt_image_info (struct pt_image *image, const struct pt_image_info **info_ptr);

/**
 * Check the given image's cache is stale - in other words, if the image needs to be update()'d.
 *
 * @return one of pt_cache_status
 */
int pt_image_status (struct pt_image *image);

/**
 * Update the given image's cache.
 *
 * @param params optional parameters to use for the update process
 */
int pt_image_update (struct pt_image *image, const struct pt_image_params *params);

/**
 * Load the image's cache in read-only mode without trying to update it.
 *
 * Fails if the cache doesn't exist.
 */
// XXX: rename to pt_image_open?
int pt_image_load (struct pt_image *image);

/**
 * Render a PNG tile to a FILE*.
 *
 * The PNG data will be written to the given stream, which will be flushed, but not closed.
 */
int pt_image_tile_file (struct pt_image *image, const struct pt_tile_info *info, FILE *out);

/**
 * Render a PNG tile to memory.
 *
 * The PNG data will be written to a malloc'd buffer.
 *
 * @param image render from image's cache
 * @param info tile parameters
 * @param buf_ptr returned heap buffer
 * @param len_ptr returned buffer length
 */
int pt_image_tile_mem (struct pt_image *image, const struct pt_tile_info *info, char **buf_ptr, size_t *len_ptr);

/**
 * Render a PNG tile to FILE* in a parralel manner.
 *
 * The PNG data will be written to \a out, which will be fclose()'d once done.
 *
 * This function may return before the PNG has been rendered.
 *
 * Fails with PT_ERR if not pt_ctx was given to pt_image_open.
 *
 * @param image render from image's cache. The cache must have been opened previously!
 * @param info tile parameters
 * @param out IO stream to write PNG data to, and close once done
 */
int pt_image_tile_async (struct pt_image *image, const struct pt_tile_info *info, FILE *out);

/**
 * Release the given pt_image without any clean shutdown
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
    PT_ERR_CACHE_VERSION,
    PT_ERR_CACHE_MUNMAP,
    PT_ERR_CACHE_CLOSE,

    PT_ERR_TILE_CLIP,

    PT_ERR_PTHREAD_CREATE,
    PT_ERR_CTX_SHUTDOWN,

    PT_ERR_ZOOM,

    PT_ERR_MAX,
};

/**
 * Translate error code to short description
 */
const char *pt_strerror (int err);

#endif
