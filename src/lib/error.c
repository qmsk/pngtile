#include "error.h"

/*
 * Mapping for error codes
 */
const char *error_names[PT_ERR_MAX] = {
    [PT_SUCCESS]                = "Success",
    [PT_ERR_MEM]                = "malloc()",

    [PT_ERR_PATH]               = "path",
    [PT_ERR_OPEN_MODE]          = "open_mode",
    
    [PT_ERR_IMG_STAT]           = "stat(.png)",
    [PT_ERR_IMG_FOPEN]          = "fopen(.png)",
    
    [PT_ERR_PNG_CREATE]         = "png_create()",
    [PT_ERR_PNG]                = "png_*()",
   
    [PT_ERR_CACHE_STAT]         = "stat(.cache)",
    [PT_ERR_CACHE_OPEN_READ]    = "open(.cache)",
    [PT_ERR_CACHE_OPEN_TMP]     = "open(.tmp)",
    [PT_ERR_CACHE_SEEK]         = "seek(.cache)",
    [PT_ERR_CACHE_READ]         = "read(.cache)",
    [PT_ERR_CACHE_WRITE]        = "write(.cache)",
    [PT_ERR_CACHE_TRUNC]        = "truncate(.cache)",
    [PT_ERR_CACHE_MMAP]         = "mmap(.cache)",
    [PT_ERR_CACHE_RENAME_TMP]   = "rename(.tmp, .cache)",

    [PT_ERR_TILE_CLIP]          = "Tile outside of image",
};

const char *pt_strerror (int err)
{
    if (err < 0)
        err = -err;

    if (err < PT_SUCCESS || err >= PT_ERR_MAX)
        return "Unknown error";
    else
        return error_names[err];
}
