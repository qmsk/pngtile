#ifndef PNGTILE_PNG_H
#define PNGTILE_PNG_H

/**
 * @file
 * PNG-specific handling
 */
#include <png.h>
#include <stdint.h>

/**
 * PNG img state
 */
struct pt_png_img {
    /** libpng state */
    png_struct *png;
    png_info *info;

    /** Possible opened I/O file */
    FILE *fh;
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

static inline size_t pt_png_data_size (const struct pt_png_header *header)
{
  // calculate data size
  return header->height * (size_t)header->row_bytes;
}

/**
 * Decode target.
 */
struct pt_png_out {
  const struct pt_png_header *header;

  uint8_t *data;

  unsigned row, col; // pixels
};

#include "tile.h"

/**
 * Check if the given path looks like a .png file.
 *
 * Returns 0 if ok, 1 if non-png file, -1 on error
 */
int pt_sniff_png (const char *path);

/**
 * Read basic image metadata.
 */
int pt_read_png_info (const char *path, struct pt_image_info *info);

/**
 * Scale PNG header for mutli-part image.
 */
 int pt_read_parts_png_header (const struct pt_image_parts *parts, struct pt_png_header *header, struct pt_png_header *part_header);

/**
 * Open the given .png image file.
 */
int pt_png_open_path (struct pt_png_img *img, const char *path);

/**
 * Read PNG from given file.
 */
int pt_png_open (struct pt_png_img *img, FILE *file);

/**
 * Read basic info from PNG header.
 */
int pt_png_read_info (struct pt_png_img *img, struct pt_image_info *info);

/**
 * Fill in the PNG header and return the size of the pixel data
 */
 int pt_png_read_header (struct pt_png_img *img, struct pt_png_header *header);

/**
 * Decode the PNG data into the given data segment, using the header as decoded by pt_png_read_header
 */
int pt_png_decode (struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params, const struct pt_png_out *out);

/**
 * Render out a tile
 */
int pt_png_tile (const struct pt_png_header *header, const uint8_t *data, struct pt_tile *tile);

/**
 * Release pt_png_ctx resources as allocated by pt_png_open
 */
void pt_png_release_read (struct pt_png_img *img);

/**
 * Release pt_png_ctx resources as allocated by pt_png_...
 */
void pt_png_release_write (struct pt_png_img *img);

#endif
