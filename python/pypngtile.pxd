from libc.stdio cimport (
        FILE,
)

cdef extern from "pngtile.h" :
    struct pt_ctx :
        pass

    struct pt_image :
        pass

    enum pt_open_mode :
        PT_OPEN_READ    # 0
        PT_OPEN_UPDATE

    enum pt_cache_status :
        PT_CACHE_ERROR  # -1
        PT_CACHE_FRESH
        PT_CACHE_NONE
        PT_CACHE_STALE
        PT_CACHE_INCOMPAT

    struct pt_image_info :
        size_t img_width, img_height, img_bpp
        int image_mtime, cache_mtime, cache_version
        size_t image_bytes, cache_bytes
        size_t cache_blocks

    struct pt_image_params :
        int background_color[4]
    
    struct pt_tile_info :
        size_t width, height
        size_t x, y
        int zoom

    ctypedef pt_image_info* const_image_info_ptr "const struct pt_image_info *"
    
    ## functions
    int pt_image_open (pt_image **image_ptr, pt_ctx *ctx, char *png_path, int cache_mode) nogil
    int pt_image_info_ "pt_image_info" (pt_image *image, pt_image_info **info_ptr) nogil
    int pt_image_status (pt_image *image) nogil
    int pt_image_load (pt_image *image) nogil
    int pt_image_update (pt_image *image, pt_image_params *params) nogil
    int pt_image_tile_file (pt_image *image, pt_tile_info *info, FILE *out) nogil
    int pt_image_tile_mem (pt_image *image, pt_tile_info *info, char **buf_ptr, size_t *len_ptr) nogil
    void pt_image_destroy (pt_image *image) nogil
    
    # error code -> name
    char* pt_strerror (int err)
