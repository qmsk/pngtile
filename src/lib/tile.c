#include "tile.h"
#include "error.h"
#include "shared/log.h" // only FATAL

#include <stdlib.h>

static void pt_tile_init (struct pt_tile *tile, const struct pt_tile_info *info, enum pt_tile_output out_type)
{
    memset(tile, 0, sizeof(*tile));
    
    // init
    tile->info = *info;
    tile->out_type = out_type;
}


int pt_tile_init_file (struct pt_tile *tile, const struct pt_tile_info *info, FILE *out)
{
    pt_tile_init(tile, info, PT_TILE_OUT_FILE);

    tile->out.file = out;

    return 0;
}

int pt_tile_init_mem (struct pt_tile *tile, const struct pt_tile_info *info)
{
    pt_tile_init(tile, info, PT_TILE_OUT_MEM);
    
    // init buffer
    if ((tile->out.mem.base = malloc(PT_TILE_BUF_SIZE)) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    tile->out.mem.len = PT_TILE_BUF_SIZE;
    tile->out.mem.off = 0;

    return 0;
}

static void pt_tile_mem_write (png_structp png, png_bytep data, png_size_t length)
{
    struct pt_tile_mem *buf = png_get_io_ptr(png);
    size_t buf_len = buf->len;

    // grow?
    while (buf->off + length > buf_len)
        buf_len *= 2;

    if (buf_len != buf->len) {
        char *tmp;

        if ((tmp = realloc(buf->base, buf_len)) == NULL)
            png_error(png, "pt_tile_buf_write - realloc failed");

        buf->base = tmp;
        buf->len = buf_len;
    }

    // copy
    memcpy(buf->base + buf->off, data, length);

    buf->off += length;
}

static void pt_tile_mem_flush (png_structp png_ptr)
{
    // no-op
}


int pt_tile_render (struct pt_tile *tile, struct pt_cache *cache)
{
    png_structp png = NULL;
    png_infop info = NULL;
    int err = 0;

    // open PNG writer
    if ((png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);
    
    if ((info = png_create_info_struct(png)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_PNG_CREATE);

    // libpng error trap
    if (setjmp(png_jmpbuf(png)))
        JUMP_SET_ERROR(err, PT_ERR_PNG);
 
    // setup output I/O
    switch (tile->out_type) {
        case PT_TILE_OUT_FILE:
            // use default FILE* operation
            png_init_io(png, tile->out.file);

            break;

        case PT_TILE_OUT_MEM:
            // use pt_tile_mem struct via pt_tile_mem_* callbacks
            png_set_write_fn(png, &tile->out.mem, pt_tile_mem_write, pt_tile_mem_flush);

            break;

        default:
            FATAL("tile->out_type: %d", tile->out_type);
    }

    // render tile
    if ((err = pt_cache_tile_png(cache, png, info, &tile->info)))
        JUMP_ERROR(err);

    // done
    png_write_end(png, info);

error:
    // cleanup
    png_destroy_write_struct(&png, &info);

    return err;
}

void pt_tile_abort (struct pt_tile *tile)
{
    // cleanup
    switch (tile->out_type) {
        case PT_TILE_OUT_FILE:
            // no-op
            break;

        case PT_TILE_OUT_MEM:
            // drop buffer
            free(tile->out.mem.base);

            break;
    }
}

