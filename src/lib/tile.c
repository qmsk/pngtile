#include "tile.h"
#include "error.h"
#include "shared/log.h" // only FATAL

#include <stdlib.h>
#include <assert.h>

int pt_tile_mem_write (struct pt_tile_mem *buf, void *data, size_t len)
{
    size_t buf_len = buf->len;

    // grow?
    while (buf->off + len > buf_len)
        buf_len *= 2;

    if (buf_len != buf->len) {
        char *tmp;

        if ((tmp = realloc(buf->base, buf_len)) == NULL)
            RETURN_ERROR(PT_ERR_MEM);

        buf->base = tmp;
        buf->len = buf_len;
    }

    // copy
    memcpy(buf->base + buf->off, data, len);

    buf->off += len;
    
    return 0;
}


int pt_tile_new (struct pt_tile **tile_ptr)
{
    struct pt_tile *tile;

    if ((tile = calloc(1, sizeof(*tile))) == NULL)
        return -PT_ERR_MEM;

    *tile_ptr = tile;

    return 0;
}

static void pt_tile_init (struct pt_tile *tile, struct pt_cache *cache, const struct pt_tile_info *info, enum pt_tile_output out_type)
{
    // init
    tile->cache = cache;
    tile->info = *info;
    tile->out_type = out_type;
}

int pt_tile_init_file (struct pt_tile *tile, struct pt_cache *cache, const struct pt_tile_info *info, FILE *out)
{
    pt_tile_init(tile, cache, info, PT_TILE_OUT_FILE);

    tile->out.file = out;

    return 0;
}

int pt_tile_init_mem (struct pt_tile *tile, struct pt_cache *cache, const struct pt_tile_info *info)
{
    pt_tile_init(tile, cache, info, PT_TILE_OUT_MEM);
    
    // init buffer
    if ((tile->out.mem.base = malloc(PT_TILE_BUF_SIZE)) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    tile->out.mem.len = PT_TILE_BUF_SIZE;
    tile->out.mem.off = 0;

    return 0;
}


int pt_tile_render (struct pt_tile *tile)
{
    // validate dimensions
    if (!tile->info.width || !tile->info.height)
        RETURN_ERROR(PT_ERR_TILE_DIM);

    return pt_cache_tile(tile->cache, tile);
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

void pt_tile_destroy (struct pt_tile *tile)
{
    pt_tile_abort(tile);

    free(tile);
}

