from libc.stdio cimport (
        FILE,
)

cdef extern from "pngtile.h" :
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
        size_t image_width, image_height, image_bpp
        int image_mtime, cache_mtime, cache_version
        size_t image_bytes, cache_bytes
        size_t cache_blocks

    ctypedef unsigned char[4] pt_image_pixel

    enum pt_image_flags :
        PT_IMAGE_BACKGROUND_PIXEL

    struct pt_image_params :
        int flags
        pt_image_pixel background_pixel

    struct pt_tile_params :
        size_t width, height
        size_t x, y
        int zoom

    ## functions
    int pt_image_new (pt_image **image_ptr, char *png_path, int cache_mode) nogil
    int pt_image_info_ "pt_image_info" (pt_image *image, pt_image_info *info_ptr) nogil
    int pt_image_status (pt_image *image) nogil
    int pt_image_open (pt_image *image) nogil
    int pt_image_update (pt_image *image, pt_image_params *params) nogil
    int pt_image_tile_file (pt_image *image, pt_tile_params *params, FILE *out) nogil
    int pt_image_tile_mem (pt_image *image, pt_tile_params *params, char **buf_ptr, size_t *len_ptr) nogil
    void pt_image_destroy (pt_image *image) nogil

    # error code -> name
    char* pt_strerror (int err)
