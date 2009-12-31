#include "cache.h"
#include "shared/util.h"
#include "shared/log.h" // only LOG_DEBUG
#include "error.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <assert.h>


int pt_cache_new (struct pt_cache **cache_ptr, const char *path, int mode)
{
    struct pt_cache *cache;
    int err;

    // alloc
    if ((cache = calloc(1, sizeof(*cache))) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    if ((cache->path = strdup(path)) == NULL)
        JUMP_SET_ERROR(err, PT_ERR_MEM);

    // init
    cache->fd = -1;
    cache->mode = mode;

    // ok
    *cache_ptr = cache;

    return 0;

error:
    // cleanup
    if (cache)
        pt_cache_destroy(cache);

    return err;
}

int pt_cache_status (struct pt_cache *cache, const char *img_path)
{
    struct stat st_img, st_cache;
    
    // test original file
    if (stat(img_path, &st_img) < 0)
        RETURN_ERROR(PT_ERR_IMG_STAT);
    
    // test cache file
    if (stat(cache->path, &st_cache) < 0) {
        // always stale if it doesn't exist yet
        if (errno == ENOENT)
            return PT_CACHE_NONE;
        else
            RETURN_ERROR(PT_ERR_CACHE_STAT);
    }

    // compare mtime
    if (st_img.st_mtime > st_cache.st_mtime)
        return PT_CACHE_STALE;

    else
        return PT_CACHE_FRESH;
}

int pt_cache_info (struct pt_cache *cache, struct pt_image_info *info)
{
    int err;

    // ensure open
    if ((err = pt_cache_open(cache)))
        return err;

    info->width = cache->header->width;
    info->height = cache->header->height;

    return 0;
}

/**
 * Abort any incomplete open operation, cleaning up
 */
static void pt_cache_abort (struct pt_cache *cache)
{
    if (cache->header != NULL) {
        munmap(cache->header, PT_CACHE_HEADER_SIZE + cache->size);

        cache->header = NULL;
        cache->data = NULL;
    }

    if (cache->fd >= 0) {
        close(cache->fd);

        cache->fd = -1;
    }
}

/**
 * Open the cache file as an fd for reading
 *
 * XXX: needs locking
 */
static int pt_cache_open_read_fd (struct pt_cache *cache, int *fd_ptr)
{
    int fd;
    
    // actual open()
    if ((fd = open(cache->path, O_RDONLY)) < 0)
        RETURN_ERROR_ERRNO(PT_ERR_OPEN_MODE, EACCES);

    // ok
    *fd_ptr = fd;

    return 0;
}

/**
 * Open the .tmp cache file as an fd for writing
 */
static int pt_cache_open_tmp_fd (struct pt_cache *cache, int *fd_ptr)
{
    int fd;
    char tmp_path[1024];
    
    // get .tmp path
    if (path_with_fext(cache->path, tmp_path, sizeof(tmp_path), ".tmp"))
        RETURN_ERROR(PT_ERR_PATH);

    // open for write, create
    // XXX: locking?
    if ((fd = open(tmp_path, O_RDWR | O_CREAT, 0644)) < 0)
        RETURN_ERROR(PT_ERR_CACHE_OPEN_TMP);

    // ok
    *fd_ptr = fd;

    return 0;
}


/**
 * Mmap the opened cache file using PT_CACHE_HEADER_SIZE plus the calculated size stored in cache->size
 */
static int pt_cache_open_mmap (struct pt_cache *cache, void **addr_ptr, bool readonly)
{
    int prot = 0;
    void *addr;

    // determine prot
    prot |= PROT_READ;

    if (!readonly) {
        assert(cache->mode & PT_OPEN_UPDATE);

        prot |= PROT_WRITE;
    }

    // perform mmap() from second page on
    if ((addr = mmap(NULL, PT_CACHE_HEADER_SIZE + cache->size, prot, MAP_SHARED, cache->fd, 0)) == MAP_FAILED)
        RETURN_ERROR(PT_ERR_CACHE_MMAP);

    // ok
    *addr_ptr = addr;

    return 0;
}

/**
 * Read in the cache header from the open file
 */
static int pt_cache_read_header (struct pt_cache *cache, struct pt_cache_header *header)
{
    size_t len = sizeof(*header);
    char *buf = (char *) header;

    // seek to start
    if (lseek(cache->fd, 0, SEEK_SET) != 0)
        RETURN_ERROR(PT_ERR_CACHE_SEEK);

    // write out full header
    while (len) {
        ssize_t ret;
        
        // try and write out the header
        if ((ret = read(cache->fd, buf, len)) <= 0)
            RETURN_ERROR(PT_ERR_CACHE_READ);

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Write out the cache header into the opened file
 */
static int pt_cache_write_header (struct pt_cache *cache, const struct pt_cache_header *header)
{
    size_t len = sizeof(*header);
    const char *buf = (const char *) header;

    // seek to start
    if (lseek(cache->fd, 0, SEEK_SET) != 0)
        RETURN_ERROR(PT_ERR_CACHE_SEEK);

    // write out full header
    while (len) {
        ssize_t ret;
        
        // try and write out the header
        if ((ret = write(cache->fd, buf, len)) <= 0)
            RETURN_ERROR(PT_ERR_CACHE_WRITE);

        // update offset
        buf += ret;
        len -= ret;
    }

    // done
    return 0;
}

/**
 * Create a new .tmp cache file, open it, and write out the header.
 */
static int pt_cache_create (struct pt_cache *cache, struct pt_cache_header *header)
{
    void *base;
    int err;

    // no access
    if (!(cache->mode & PT_OPEN_UPDATE))
        RETURN_ERROR(PT_ERR_OPEN_MODE);

    // open as .tmp
    if ((err = pt_cache_open_tmp_fd(cache, &cache->fd)))
        return err;

    // calculate data size
    cache->size = sizeof(*header) + header->height * header->row_bytes;

    // grow file
    if (ftruncate(cache->fd, PT_CACHE_HEADER_SIZE + cache->size) < 0)
        JUMP_SET_ERROR(err, PT_ERR_CACHE_TRUNC);

    // mmap header and data
    if ((err = pt_cache_open_mmap(cache, &base, false)))
        JUMP_ERROR(err);

    cache->header = base;
    cache->data = base + PT_CACHE_HEADER_SIZE;

    // write header
    // XXX: should just mmap...
    if ((err = pt_cache_write_header(cache, header)))
        JUMP_ERROR(err);

    // done
    return 0;

error:
    // cleanup
    pt_cache_abort(cache);

    return err;
}

/**
 * Rename the opened .tmp to .cache
 */
static int pt_cache_create_done (struct pt_cache *cache)
{
    char tmp_path[1024];
    
    // get .tmp path
    if (path_with_fext(cache->path, tmp_path, sizeof(tmp_path), ".tmp"))
        RETURN_ERROR(PT_ERR_PATH);

    // rename
    if (rename(tmp_path, cache->path) < 0)
        RETURN_ERROR(PT_ERR_CACHE_RENAME_TMP);

    // ok
    return 0;
}

int pt_cache_open (struct pt_cache *cache)
{
    struct pt_cache_header header;
    void *base;
    int err;

    // ignore if already open
    if (cache->header && cache->data)
        return 0;

    // open the .cache
    if ((err = pt_cache_open_read_fd(cache, &cache->fd)))
        return err;

    // read in header
    if ((err = pt_cache_read_header(cache, &header)))
        JUMP_ERROR(err);

    // calculate data size
    cache->size = sizeof(header) + header.height * header.row_bytes;

    // mmap header and data
    if ((err = pt_cache_open_mmap(cache, &base, true)))
        JUMP_ERROR(err);

    cache->header = base;
    cache->data = base + PT_CACHE_HEADER_SIZE;

    // done
    return 0;

error:
    // cleanup
    pt_cache_abort(cache);

    return err;
}

int pt_cache_update_png (struct pt_cache *cache, png_structp png, png_infop info)
{
    struct pt_cache_header header;
    int err;
    
    // XXX: check cache_mode
    // XXX: check image doesn't use any options we don't handle
    // XXX: close any already-opened cache file

    memset(&header, 0, sizeof(header));

    // fill in basic info
    header.width = png_get_image_width(png, info);
    header.height = png_get_image_height(png, info);
    header.bit_depth = png_get_bit_depth(png, info);
    header.color_type = png_get_color_type(png, info);

    log_debug("width=%u, height=%u, bit_depth=%u, color_type=%u", 
            header.width, header.height, header.bit_depth, header.color_type
    );

    // only pack 1 pixel per byte, changes rowbytes
    if (header.bit_depth < 8)
        png_set_packing(png);

    // fill in other info
    header.row_bytes = png_get_rowbytes(png, info);

    // calculate bpp as num_channels * bpc
    // XXX: this assumes the packed bit depth will be either 8 or 16
    header.col_bytes = png_get_channels(png, info) * (header.bit_depth == 16 ? 2 : 1);

    log_debug("row_bytes=%u, col_bytes=%u", header.row_bytes, header.col_bytes);
    
    // palette etc.
    if (header.color_type == PNG_COLOR_TYPE_PALETTE) {
        int num_palette;
        png_colorp palette;

        if (png_get_PLTE(png, info, &palette, &num_palette) == 0)
            // XXX: PLTE chunk not read?
            RETURN_ERROR(PT_ERR_PNG);
        
        // should only be 256 of them at most
        assert(num_palette <= PNG_MAX_PALETTE_LENGTH);
    
        // copy
        header.num_palette = num_palette;
        memcpy(&header.palette, palette, num_palette * sizeof(*palette));
        
        log_debug("num_palette=%u", num_palette);
    }

    // create .tmp and write out header
    if ((err = pt_cache_create(cache, &header)))
        return err;


    // write out raw image data a row at a time
    for (size_t row = 0; row < header.height; row++) {
        // read row data, non-interlaced
        png_read_row(png, cache->data + row * header.row_bytes, NULL);
    }


    // move from .tmp to .cache
    if ((err = pt_cache_create_done(cache)))
        // XXX: pt_cache_abort?
        return err;

    // done!
    return 0;
}

/**
 * Return a pointer to the pixel data on \a row, starting at \a col.
 */
static inline void* tile_row_col (struct pt_cache *cache, size_t row, size_t col)
{
    return cache->data + (row * cache->header->row_bytes) + (col * cache->header->col_bytes);
}

/**
 * Fill in a clipped region of \a width_px pixels at the given row segment
 */
static inline void tile_row_fill_clip (struct pt_cache *cache, png_byte *row, size_t width_px)
{
    // XXX: use a configureable background color, or full transparency?
    memset(row, /* 0xd7 */ 0x00, width_px * cache->header->col_bytes);
}

/**
 * Write raw tile image data, directly from the cache
 */
static int write_png_data_direct (struct pt_cache *cache, png_structp png, png_infop info, const struct pt_tile_info *ti)
{
    for (size_t row = ti->y; row < ti->y + ti->height; row++)
        // write data directly
        png_write_row(png, tile_row_col(cache, row, ti->x));

    return 0;
}

/**
 * Write clipped tile image data (a tile that goes over the edge of the actual image) by aligning the data from the cache as needed
 */
static int write_png_data_clipped (struct pt_cache *cache, png_structp png, png_infop info, const struct pt_tile_info *ti)
{
    png_byte *rowbuf;
    size_t row;
    
    // image data goes from (ti->x ... clip_x, ti->y ... clip_y), remaining region is filled
    size_t clip_x, clip_y;


    // figure out if the tile clips over the right edge
    // XXX: use min()
    if (ti->x + ti->width > cache->header->width)
        clip_x = cache->header->width;
    else
        clip_x = ti->x + ti->width;
    
    // figure out if the tile clips over the bottom edge
    // XXX: use min()
    if (ti->y + ti->height > cache->header->height)
        clip_y = cache->header->height;
    else
        clip_y = ti->y + ti->height;


    // allocate buffer for a single row of image data
    if ((rowbuf = malloc(ti->width * cache->header->col_bytes)) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    // how much data we actually have for each row, in px and bytes
    // from [(tile x)---](clip x)
    size_t row_px = clip_x - ti->x;
    size_t row_bytes = row_px * cache->header->col_bytes;

    // write the rows that we have
    // from [(tile y]---](clip y)
    for (row = ti->y; row < clip_y; row++) {
        // copy in the actual tile data...
        memcpy(rowbuf, tile_row_col(cache, row, ti->x), row_bytes);

        // generate the data for the remaining, clipped, columns
        tile_row_fill_clip(cache, rowbuf + row_bytes, (ti->width - row_px));

        // write
        png_write_row(png, rowbuf);
    }

    // generate the data for the remaining, clipped, rows
    tile_row_fill_clip(cache, rowbuf, ti->width);
    
    // write out the remaining rows as clipped data
    for (; row < ti->y + ti->height; row++)
        png_write_row(png, rowbuf);

    // ok
    return 0;
}

int pt_cache_tile_png (struct pt_cache *cache, png_structp png, png_infop info, const struct pt_tile_info *ti)
{
    int err;

    // ensure open
    if ((err = pt_cache_open(cache)))
        return err;

    // check within bounds
    if (ti->x >= cache->header->width || ti->y >= cache->header->height)
        // completely outside
        RETURN_ERROR(PT_ERR_TILE_CLIP);

    // set basic info
    png_set_IHDR(png, info, ti->width, ti->height, cache->header->bit_depth, cache->header->color_type,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );

    // set palette?
    if (cache->header->color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_PLTE(png, info, cache->header->palette, cache->header->num_palette);

    // write meta-info
    png_write_info(png, info);

    // our pixel data is packed into 1 pixel per byte (8bpp or 16bpp)
    png_set_packing(png);
    
    // figure out if the tile clips
    if (ti->x + ti->width <= cache->header->width && ti->y + ti->height <= cache->header->height)
        // doesn't clip, just use the raw data
        err = write_png_data_direct(cache, png, info, ti);

    else
        // fill in clipped regions
        err = write_png_data_clipped(cache, png, info, ti);

    if (err)
        return err;
    
    // done, flush remaining output
    png_write_flush(png);

    // ok
    return 0;
}

void pt_cache_destroy (struct pt_cache *cache)
{
    free(cache->path);

    pt_cache_abort(cache);

    free(cache);
}

