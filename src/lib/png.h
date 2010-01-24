#ifndef PNGTILE_PNG_H
#define PNGTILE_PNG_H

/**
 * @file
 * PNG-specific handling
 */
#include <png.h>
#include <stdint.h>

/**
 * Handle sparse data at this granularity (pixels)
 */
#define PT_IMG_BLOCK_SIZE 64

/**
 * PNG img state
 */
struct pt_png_img {
    /** libpng state */
    png_struct *png;
    png_info *info;

};

/**
 * Cache header layout for PNG-format images
 */
struct pt_png_header {
    /** Pixel dimensions of image */
    uint32_t width, height;
    
    /** Pixel format */
    uint8_t bit_depth, color_type;

    /** Number of png_color entries that follow */
    uint16_t num_palette;

    /** Number of bytes per row */
    uint32_t row_bytes;
    
    /** Number of bytes per pixel */
    uint8_t col_bytes;

    /** Palette entries, up to 256 entries used */
    png_color palette[PNG_MAX_PALETTE_LENGTH];
};


#include "image.h"
#include "tile.h"


/**
 * Open the given .png image, and read info
 */
int pt_png_open (struct pt_image *image, struct pt_png_img *img);

/**
 * Fill in the PNG header and return the size of the pixel data
 */
int pt_png_read_header (struct pt_png_img *img, struct pt_png_header *header, size_t *data_size);

/**
 * Decode the PNG data into the given data segment, using the header as decoded by pt_png_read_header
 */
int pt_png_decode (struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params, uint8_t *out);

/**
 * Fill in img_* fields of pt_image_info from header
 */
int pt_png_info (struct pt_png_header *header, struct pt_image_info *info);

/**
 * Render out a tile
 */
int pt_png_tile (struct pt_png_header *header, uint8_t *data, struct pt_tile *tile);

/**
 * Release pt_png_ctx resources as allocated by pt_png_open
 */
void pt_png_release_read (struct pt_png_img *img);

/**
 * Release pt_png_ctx resources as allocated by pt_png_...
 */
void pt_png_release_write (struct pt_png_img *img);

#endif
