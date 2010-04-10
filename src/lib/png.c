#include "png.h" // pt_png header
#include "error.h"
#include "shared/log.h" // debug only

#include <png.h> // sysmtem libpng header
#include <assert.h>


#define min(a, b) (((a) < (b)) ? (a) : (b))

int pt_png_check (const char *path)
{
    FILE *fp;
    uint8_t header[8];
    int ret;

    // fopen
    if ((fp = fopen(path, "rb")) == NULL)
        RETURN_ERROR(PT_ERR_IMG_OPEN);

    // read
    if (fread(header, 1, sizeof(header), fp) != sizeof(header))
        JUMP_SET_ERROR(ret, PT_ERR_IMG_FORMAT);
      
    // compare signature  
    if (png_sig_cmp(header, 0, sizeof(header)))
        // not a PNG file
        ret = 1;

    else
        // valid PNG file
        ret = 0;

error:
    // cleanup
    fclose(fp);

    return ret;
}

int pt_png_open (struct pt_image *image, struct pt_png_img *img)
{
    int err;

    // init
    memset(img, 0, sizeof(*img));
    
    // open I/O
    if ((err = pt_image_open_file(image, &img->fh)))
        JUMP_ERROR(err);

    // create the struct
    if ((img->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);

    // create the info
    if ((img->info = png_create_info_struct(img->png)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);

    // setup error trap for the I/O
    if (setjmp(png_jmpbuf(img->png)))
        JUMP_SET_ERROR(err, PT_ERR_PNG);
    
    // setup error trap
    if (setjmp(png_jmpbuf(img->png)))
        JUMP_SET_ERROR(err, PT_ERR_PNG);
    

    // setup I/O to FILE
    png_init_io(img->png, img->fh);

    // read meta-info
    png_read_info(img->png, img->info);


    // img->fh will be closed by pt_png_release_read
    return 0;

error:
    // cleanup
    pt_png_release_read(img);
    
    return err;
}

int pt_png_read_header (struct pt_png_img *img, struct pt_png_header *header, size_t *data_size)
{
    // check image doesn't use any options we don't handle
    if (png_get_interlace_type(img->png, img->info) != PNG_INTERLACE_NONE) {
        log_warn("Can't handle interlaced PNG");

        RETURN_ERROR(PT_ERR_IMG_FORMAT);
    }


    // initialize
    memset(header, 0, sizeof(*header));

    // fill in basic info
    header->width = png_get_image_width(img->png, img->info);
    header->height = png_get_image_height(img->png, img->info);
    header->bit_depth = png_get_bit_depth(img->png, img->info);
    header->color_type = png_get_color_type(img->png, img->info);

    log_debug("width=%u, height=%u, bit_depth=%u, color_type=%u", 
            header->width, header->height, header->bit_depth, header->color_type
    );

    // only pack 1 pixel per byte, changes rowbytes
    if (header->bit_depth < 8)
        png_set_packing(img->png);

    // fill in other info
    header->row_bytes = png_get_rowbytes(img->png, img->info);

    // calculate bpp as num_channels * bpc
    // this assumes the packed bit depth will be either 8 or 16
    header->col_bytes = png_get_channels(img->png, img->info) * (header->bit_depth == 16 ? 2 : 1);

    log_debug("row_bytes=%u, col_bytes=%u", header->row_bytes, header->col_bytes);
    
    // palette etc.
    if (header->color_type == PNG_COLOR_TYPE_PALETTE) {
        int num_palette;
        png_colorp palette;

        if (png_get_PLTE(img->png, img->info, &palette, &num_palette) == 0)
            // PLTE chunk not read?
            RETURN_ERROR(PT_ERR_PNG);
        
        // should only be 256 of them at most
        assert(num_palette <= PNG_MAX_PALETTE_LENGTH);
    
        // copy
        header->num_palette = num_palette;
        memcpy(&header->palette, palette, num_palette * sizeof(*palette));
        
        log_debug("num_palette=%u", num_palette);
    }

    // calculate data size
    *data_size = header->height * header->row_bytes;
    
    return 0;
}

/**
 * Decode the PNG data directly to memory - not good for sparse backgrounds
 */
static int pt_png_decode_direct (struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params, uint8_t *out)
{
    // write out raw image data a row at a time
    for (size_t row = 0; row < header->height; row++) {
        // read row data, non-interlaced
        png_read_row(img->png, out + row * header->row_bytes, NULL);
    }

    return 0;
}

/**
 * Decode the PNG data, filtering it for sparse regions
 */
static int pt_png_decode_sparse (struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params, uint8_t *out)
{
    // one row of pixel data
    uint8_t *row_buf;

    // alloc
    if ((row_buf = malloc(header->row_bytes)) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    // decode each row at a time
    for (size_t row = 0; row < header->height; row++) {
        // read row data, non-interlaced
        png_read_row(img->png, row_buf, NULL);
        
        // skip background-colored regions to keep the cache file sparse
        // ...in blocks of PT_CACHE_BLOCK_SIZE bytes
        for (size_t col_base = 0; col_base < header->width; col_base += PT_IMG_BLOCK_SIZE) {
            // size of this block in bytes
            size_t block_size = min(PT_IMG_BLOCK_SIZE * header->col_bytes, header->row_bytes - col_base);

            // ...each pixel
            for (
                    size_t col = col_base;

                    // BLOCK_SIZE * col_bytes wide, don't go over the edge
                    col < col_base + block_size; 

                    col += header->col_bytes
            ) {
                // test this pixel
                if (bcmp(row_buf + col, params->background_color, header->col_bytes)) {
                    // differs
                    memcpy(
                            out + row * header->row_bytes + col_base, 
                            row_buf + col_base,
                            block_size
                    );
                    
                    // skip to next block
                    break;
                }
            }

            // skip this block
            continue;
        }
    }

    return 0;
}

int pt_png_decode (struct pt_png_img *img, const struct pt_png_header *header, const struct pt_image_params *params, uint8_t *out)
{
    int err;

    // decode
    // XXX: it's an array, you silly
    if (params->background_color)
        err = pt_png_decode_sparse(img, header, params, out);

    else
        err = pt_png_decode_direct(img, header, params, out);
    
    if (err)
        return err;
    
    // finish off, ignore trailing data
    png_read_end(img->png, NULL);

    return 0;
}

int pt_png_info (struct pt_png_header *header, struct pt_image_info *info)
{
    // fill in info from header
    info->img_width = header->width;
    info->img_height = header->height;
    info->img_bpp = header->bit_depth;

    return 0;
}

/** 
 * libpng I/O callback: write out data
 */
static void pt_png_mem_write (png_structp png, png_bytep data, png_size_t length)
{
    struct pt_tile_mem *buf = png_get_io_ptr(png);
    int err;
    
    // write to buffer
    if ((err = pt_tile_mem_write(buf, data, length)))
        // drop err, because png_error doesn't do formatted output
        png_error(png, "pt_tile_mem_write: ...");
}

/** 
 * libpng I/O callback: flush buffered data
 */
static void pt_png_mem_flush (png_structp png_ptr)
{
    // no-op
}


/**
 * Return a pointer to the pixel data on \a row, starting at \a col.
 */
static inline const void* tile_row_col (const struct pt_png_header *header, const uint8_t *data, size_t row, size_t col)
{
    return data + (row * header->row_bytes) + (col * header->col_bytes);
}

/**
 * Write raw tile image data, directly from the cache
 */
static int pt_png_encode_direct (struct pt_png_img *img, const struct pt_png_header *header, const uint8_t *data, const struct pt_tile_info *ti)
{
    for (size_t row = ti->y; row < ti->y + ti->height; row++)
        // write data directly
        // missing const...
        png_write_row(img->png, (const png_bytep) tile_row_col(header, data, row, ti->x));

    return 0;
}

/**
 * Fill in a clipped region of \a width_px pixels at the given row segment
 */
static inline void tile_row_fill_clip (const struct pt_png_header *header, png_byte *row, size_t width_px)
{
    // XXX: use a configureable background color, or full transparency?
    memset(row, /* 0xd7 */ 0x00, width_px * header->col_bytes);
}

/**
 * Write clipped tile image data (a tile that goes over the edge of the actual image) by aligning the data from the cache as needed
 */
static int pt_png_encode_clipped (struct pt_png_img *img, const struct pt_png_header *header, const uint8_t *data, const struct pt_tile_info *ti)
{
    png_byte *rowbuf;
    size_t row;
    
    // image data goes from (ti->x ... clip_x, ti->y ... clip_y), remaining region is filled
    size_t clip_x, clip_y;


    // fit the left/bottom edge against the image dimensions
    clip_x = min(ti->x + ti->width, header->width);
    clip_y = min(ti->y + ti->height, header->height);


    // allocate buffer for a single row of image data
    if ((rowbuf = malloc(ti->width * header->col_bytes)) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    // how much data we actually have for each row, in px and bytes
    // from [(tile x)---](clip x)
    size_t row_px = clip_x - ti->x;
    size_t row_bytes = row_px * header->col_bytes;

    // write the rows that we have
    // from [(tile y]---](clip y)
    for (row = ti->y; row < clip_y; row++) {
        // copy in the actual tile data...
        memcpy(rowbuf, tile_row_col(header, data, row, ti->x), row_bytes);

        // generate the data for the remaining, clipped, columns
        tile_row_fill_clip(header, rowbuf + row_bytes, (ti->width - row_px));

        // write
        png_write_row(img->png, rowbuf);
    }

    // generate the data for the remaining, clipped, rows
    tile_row_fill_clip(header, rowbuf, ti->width);
    
    // write out the remaining rows as clipped data
    for (; row < ti->y + ti->height; row++)
        png_write_row(img->png, rowbuf);

    // ok
    return 0;
}

/**
 * Write unscaled tile data
 */
static int pt_png_encode_unzoomed (struct pt_png_img *img, const struct pt_png_header *header, const uint8_t *data, const struct pt_tile_info *ti)
{
    int err;

    // set basic info
    png_set_IHDR(img->png, img->info, ti->width, ti->height, header->bit_depth, header->color_type,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );

    // set palette?
    if (header->color_type == PNG_COLOR_TYPE_PALETTE)
        // oops... missing const
        png_set_PLTE(img->png, img->info, (png_colorp) header->palette, header->num_palette);

    // write meta-info
    png_write_info(img->png, img->info);

    // our pixel data is packed into 1 pixel per byte (8bpp or 16bpp)
    png_set_packing(img->png);
    
    // figure out if the tile clips
    if (ti->x + ti->width <= header->width && ti->y + ti->height <= header->height)
        // doesn't clip, just use the raw data
        err = pt_png_encode_direct(img, header, data, ti);

    else
        // fill in clipped regions
        err = pt_png_encode_clipped(img, header, data, ti);
    
    return err;
}

/**
 * Manipulate powers of two
 */
static inline size_t scale_by_zoom_factor (size_t value, int z)
{
    if (z > 0)
        return value << z;

    else if (z < 0)
        return value >> -z;

    else
        return value;
}

#define ADD_AVG(l, r) (l) = ((l) + (r)) / 2

/**
 * Converts a pixel's data into a png_color
 */
static inline void png_pixel_data (const png_color **outp, const struct pt_png_header *header, const uint8_t *data, size_t row, size_t col)
{
    // palette entry number
    int p;

    switch (header->color_type) {
        case PNG_COLOR_TYPE_PALETTE:
            switch (header->bit_depth) {
                case 8:
                    // 8bpp palette
                    p = *((uint8_t *) tile_row_col(header, data, row, col));

                    break;
                
                default :
                    // unknown
                    return;
            }

            // hrhr - assume our working data is valid (or we have 255 palette entries, so it doesn't matter...)
            assert(p < header->num_palette);
            
            // reference data from palette
            *outp = &header->palette[p];
            
            return;

        default :
            // unknown pixel format
            return;
    }
}

/**
 * Write scaled tile data
 */
static int pt_png_encode_zoomed (struct pt_png_img *img, const struct pt_png_header *header, const uint8_t *data, const struct pt_tile_info *ti)
{
    // size of the image data in px
    size_t data_width = scale_by_zoom_factor(ti->width, ti->zoom);
    size_t data_height = scale_by_zoom_factor(ti->height, ti->zoom);

    // input pixels per output pixel
    size_t pixel_size = scale_by_zoom_factor(1, ti->zoom);

    // bytes per output pixel
    size_t pixel_bytes = 3;

    // size of the output tile in px
    size_t row_width = ti->width;

    // size of an output row in bytes (RGB)
    size_t row_bytes = row_width * 3;

    // buffer to hold output rows
    uint8_t *row_buf;
                
    // color entry for pixel
    const png_color *c = &header->palette[0];
    
    // only supports zooming out...
    if (ti->zoom < 0)
        RETURN_ERROR(PT_ERR_TILE_ZOOM);

    if ((row_buf = malloc(row_bytes)) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    // suppress warning...
    (void) data_height;

    // define pixel format: 8bpp RGB
    png_set_IHDR(img->png, img->info, ti->width, ti->height, 8, PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );
    
    // write meta-info
    png_write_info(img->png, img->info);

    // ...each output row
    for (size_t out_row = 0; out_row < ti->height; out_row++) {
        memset(row_buf, 0, row_bytes);

        // ...includes pixels starting from this row.
        size_t in_row_offset = ti->y + scale_by_zoom_factor(out_row, ti->zoom);
        
        // ...each out row includes pixel_size in rows
        for (size_t in_row = in_row_offset; in_row < in_row_offset + pixel_size && in_row < header->height; in_row++) {
            // and includes each input pixel
            for (size_t in_col = ti->x; in_col < ti->x + data_width && in_col < header->width; in_col++) {

                // ...for this output pixel
                size_t out_col = scale_by_zoom_factor(in_col - ti->x, -ti->zoom);
                
                // get pixel RGB data
                png_pixel_data(&c, header, data, in_row, in_col);
                
                // average the RGB data        
                ADD_AVG(row_buf[out_col * pixel_bytes + 0], c->red);
                ADD_AVG(row_buf[out_col * pixel_bytes + 1], c->green);
                ADD_AVG(row_buf[out_col * pixel_bytes + 2], c->blue);
            }
        }

        // output
        png_write_row(img->png, row_buf);
    }

    // done
    return 0;
}

int pt_png_tile (const struct pt_png_header *header, const uint8_t *data, struct pt_tile *tile)
{
    struct pt_png_img _img, *img = &_img;
    struct pt_tile_info *ti = &tile->info;
    int err;

    // init img
    memset(img, 0, sizeof(*img));

    // check within bounds
    if (ti->x >= header->width || ti->y >= header->height)
        // completely outside
        RETURN_ERROR(PT_ERR_TILE_CLIP);
    
    // open PNG writer
    if ((img->png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);
    
    if ((img->info = png_create_info_struct(img->png)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);

    // libpng error trap
    if (setjmp(png_jmpbuf(img->png)))
        JUMP_SET_ERROR(err, PT_ERR_PNG);
 


    // setup output I/O
    switch (tile->out_type) {
        case PT_TILE_OUT_FILE:
            // use default FILE* operation
            // do NOT store in img->fh
            png_init_io(img->png, tile->out.file);

            break;

        case PT_TILE_OUT_MEM:
            // use pt_tile_mem struct via pt_png_mem_* callbacks
            png_set_write_fn(img->png, &tile->out.mem, pt_png_mem_write, pt_png_mem_flush);

            break;

        default:
            FATAL("tile->out_type: %d", tile->out_type);
    }


  
    // unscaled or scaled?
    if (ti->zoom)
        err = pt_png_encode_zoomed(img, header, data, ti);

    else
        err = pt_png_encode_unzoomed(img, header, data, ti);

    if (err)
        goto error;
    

    // flush remaining output
    png_write_flush(img->png);

    // done
    png_write_end(img->png, img->info);

error:
    // cleanup
    pt_png_release_write(img);

    return err;
}


void pt_png_release_read (struct pt_png_img *img)
{
    png_destroy_read_struct(&img->png, &img->info, NULL);
    
    // close possible filehandle
    if (img->fh) {
        if (fclose(img->fh))
            log_warn_errno("fclose");
    }
}

void pt_png_release_write (struct pt_png_img *img)
{
    png_destroy_write_struct(&img->png, &img->info);

    // close possible filehandle
    if (img->fh) {
        if (fclose(img->fh))
            log_warn_errno("fclose");
    }

}

