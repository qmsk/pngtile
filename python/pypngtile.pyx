cdef extern from "errno.h" :
    extern int errno

cdef extern from "string.h" :
    char* strerror (int err)

    void* memset (void *, int, size_t)
    void* memcpy (void *, void *, size_t)

cimport stdio
cimport stdlib
cimport python_string

cdef extern from "Python.h" :
    int PyFile_Check (object p)
    stdio.FILE* PyFile_AsFile (object p)
    void PyFile_IncUseCount (object p)
    void PyFile_DecUseCount (object p)

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

    int pt_image_open (pt_image **image_ptr, pt_ctx *ctx, char *png_path, int cache_mode)
    int pt_image_info_func "pt_image_info" (pt_image *image, pt_image_info **info_ptr)
    int pt_image_status (pt_image *image)
    int pt_image_update (pt_image *image, pt_image_params *params)
    int pt_image_tile_file (pt_image *image, pt_tile_info *info, stdio.FILE *out)
    int pt_image_tile_mem (pt_image *image, pt_tile_info *info, char **buf_ptr, size_t *len_ptr)
    void pt_image_destroy (pt_image *image)

    char* pt_strerror (int err)

## constants
# Image()
OPEN_READ       = PT_OPEN_READ
OPEN_UPDATE     = PT_OPEN_UPDATE

# Image.status -> ...
CACHE_FRESH     = PT_CACHE_FRESH
CACHE_NONE      = PT_CACHE_NONE
CACHE_STALE     = PT_CACHE_STALE
CACHE_INCOMPAT  = PT_CACHE_INCOMPAT

class Error (BaseException) :
    pass

cdef int trap_err (char *op, int ret) except -1 :
    if ret < 0 :
        raise Error("%s: %s: %s" % (op, pt_strerror(ret), strerror(errno)))

    else :
        return ret

cdef class Image :
    """
        An image file on disk (.png) and an associated .cache file.
        
        Open the .png file at the given path using the given mode.

        path        - filesystem path to .png file
        mode        - mode to operate cache in
            OPEN_READ       - read-only access to cache
            OPEN_UPDATE     - allow .update()
    """

    cdef pt_image *image

    
    # open the pt_image
    def __cinit__ (self, char *path, int mode = 0) :
        trap_err("pt_image_open", 
            pt_image_open(&self.image, NULL, path, mode)
        )


    def info (self) :
        """
            Return a dictionary containing various information about the image.

            img_width           - pixel dimensions of the source image
            img_height            only available if the cache was opened
            img_bpp             - bits per pixel for the source image

            image_mtime         - last modification timestamp for source image
            image_bytes         - size of source image file in bytes

            cache_version       - version of cache file available
            cache_mtime         - last modification timestamp for cache file
            cache_bytes         - size of cache file in bytes
            cache_blocks        - size of cache file in disk blocks - 512 bytes / block
        """

        cdef pt_image_info *info
        
        trap_err("pt_image_info",
            pt_image_info_func(self.image, &info)
        )

        # return as a struct
        return info[0]


    def status (self) :
        """
            Return a code describing the status of the underlying cache file.

            CACHE_FRESH         - the cache file exists and is up-to-date
            CACHE_NONE          - the cache file does not exist
            CACHE_STALE         - the cache file exists, but is older than the source image
            CACHE_INCOMPAT      - the cache file exists, but is incompatible with this version of the library
        """

        return trap_err("pt_image_status", 
            pt_image_status(self.image)
        )


    def update (self, background_color = None) :
        """
            Update the underlying cache file from the source image.

            background_color    - skip consecutive pixels that match this byte pattern in output

            Requires that the Image was opened using OPEN_UPDATE.
        """

        cdef pt_image_params params
        cdef char *bgcolor
        memset(&params, 0, sizeof(params))

        # params
        if background_color :
            # cast
            bgcolor = <char *>background_color

            if 0 >= len(bgcolor) > 4 :
                raise ValueError("background_color must be a str of between 1 and 4 bytes")

            # decode 
            memcpy(params.background_color, bgcolor, len(bgcolor))
    
        # run update
        trap_err("pt_image_update", 
            pt_image_update(self.image, &params)
        )


    def tile_file (self, size_t width, size_t height, size_t x, size_t y, int zoom, object out) :
        cdef stdio.FILE *outf
        cdef pt_tile_info ti

        if not PyFile_Check(out) :
            raise TypeError("out: must be a file object")

        outf = PyFile_AsFile(out)

        if not outf :
            raise TypeError("out: must have a FILE*")
    
        ti.width = width
        ti.height = height
        ti.x = x
        ti.y = y
        ti.zoom = zoom
        
        trap_err("pt_image_tile_file", 
            pt_image_tile_file(self.image, &ti, outf)
        )


    def tile_mem (self, size_t width, size_t height, size_t x, size_t y, int zoom) :
        cdef pt_tile_info ti
        cdef char *buf
        cdef size_t len

        ti.width = width
        ti.height = height
        ti.x = x
        ti.y = y
        ti.zoom = zoom
        
        # render and return ptr to buffer
        trap_err("pt_image_tile_mem", 
            pt_image_tile_mem(self.image, &ti, &buf, &len)
        )
        
        # copy buffer as str...
        data = python_string.PyString_FromStringAndSize(buf, len)

        # drop buffer...
        stdlib.free(buf)

        return data

    # release the pt_image
    def __dealloc__ (self) :
        if self.image :
            pt_image_destroy(self.image)

