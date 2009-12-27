#include "cache.h"

static int pt_cache_new (struct pt_cache **cache_ptr)
{
    struct pt_cache *cache;

    if ((cache = calloc(1, sizeof(*cache))) == NULL)
        return -1;

    // ok
    *cache_ptr = cache;

    return 0;
}

int pt_cache_open (struct pt_cache **cache_ptr, struct pt_image *img, int mode)
{
    struct pt_cache *cache;
    
    // alloc
    if (pt_cache_new(&cache))
        return -1;

    
}

bool pt_cache_fresh (struct pt_cache *cache)
{
    // TODO: stat + mtime
    return false;
}

/**
 * Create a new cache file, open it, and write out the header.
 */
static int pt_cache_create (struct pt_cache *cache, struct pt_cache_header *header)
{
    
}

int pt_cache_update_png (struct pt_cache *cache, png_structp png, png_infop info)
{
    struct pt_cache_header header;

    memset(&header, 0, sizeof(header));

    // fill in basic info
    header->width = png_get_image_width(png, info);
    header->height = png_get_image_height(png, info);
    header->bit_depth = png_get_bit_depth(png, info);
    header->color_type = png_get_color_type(png, info);

    // fill in other info
    header->row_bytes = png_get_rowbytes(png, info);
}
