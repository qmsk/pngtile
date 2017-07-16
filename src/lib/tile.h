#ifndef PNGTILE_TILE_H
#define PNGTILE_TILE_H

#include "pngtile.h"

/** Types of tile output */
enum pt_tile_output {
    PT_TILE_OUT_FILE,
    PT_TILE_OUT_MEM,
};

/** Initial size of out.mem.base, 16k */
#define PT_TILE_BUF_SIZE (16 * 1024)

/** Per-tile-render state */
struct pt_tile {
    /** Render spec */
    struct pt_tile_params params;

    /** Output type */
    enum pt_tile_output out_type;

    union {
        /** Output file */
        FILE *file;

        /** Output buffer */
        struct pt_tile_mem {
            char *base;
            size_t off, len;
        } mem;
    } out;
};

/**
 * Write to the tile's output buffer
 */
int pt_tile_mem_write (struct pt_tile_mem *buf, void *data, size_t len);

/**
 * Alloc a new pt_tile, which must be initialized using pt_tile_init_*
 */
int pt_tile_new (struct pt_tile **tile_ptr);

/**
 * Initialize to render with given params, writing output to given FILE*
 */
int pt_tile_init_file (struct pt_tile *tile, const struct pt_tile_params *params, FILE *out);

/**
 * Initialize to render with given params, writing output to a memory buffer
 */
int pt_tile_init_mem (struct pt_tile *tile, const struct pt_tile_params *params);

/**
 * Abort any failed render process, cleaning up.
 */
void pt_tile_abort (struct pt_tile *tile);

/**
 * Destroy given pt_tile, aborting it and freeing it
 */
void pt_tile_destroy (struct pt_tile *tile);

#endif
