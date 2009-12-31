cdef extern from "errno.h" :
    extern int errno

cdef extern from "string.h" :
    char* strerror (int err)

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
        PT_OPEN_UPDATE

    enum pt_cache_status :
        PT_CACHE_ERROR
        PT_CACHE_FRESH
        PT_CACHE_NONE
        PT_CACHE_STALE

    struct pt_image_info :
        size_t width, height

    struct pt_tile_info :
        size_t width, height
        size_t x, y
        
    int pt_image_open (pt_image **image_ptr, pt_ctx *ctx, char *png_path, int cache_mode)
    int pt_image_info_func "pt_image_info" (pt_image *image, pt_image_info **info_ptr)
    int pt_image_status (pt_image *image)
    int pt_image_update (pt_image *image)
    int pt_image_tile_file (pt_image *image, pt_tile_info *info, stdio.FILE *out)
    int pt_image_tile_mem (pt_image *image, pt_tile_info *info, char **buf_ptr, size_t *len_ptr)
    void pt_image_destroy (pt_image *image)

    char* pt_strerror (int err)

OPEN_UPDATE = PT_OPEN_UPDATE
CACHE_ERROR = PT_CACHE_ERROR
CACHE_FRESH = PT_CACHE_FRESH
CACHE_NONE = PT_CACHE_NONE
CACHE_STALE = PT_CACHE_STALE

class Error (BaseException) :
    pass

cdef int trap_err (char *op, int ret) except -1 :
    if ret < 0 :
        raise Error("%s: %s: %s" % (op, pt_strerror(ret), strerror(errno)))

    else :
        return ret

cdef class Image :
    cdef pt_image *image

    def __cinit__ (self, char *png_path, int cache_mode = 0) :
        trap_err("pt_image_open", 
            pt_image_open(&self.image, NULL, png_path, cache_mode)
        )
    
    def info (self) :
        cdef pt_image_info *image_info
        
        trap_err("pt_image_info",
            pt_image_info_func(self.image, &image_info)
        )

        return (image_info.width, image_info.height)
    
    def status (self) :
        return trap_err("pt_image_status", 
            pt_image_status(self.image)
        )
    
    def update (self) :
        trap_err("pt_image_update", 
            pt_image_update(self.image)
        )

    def tile_file (self, size_t width, size_t height, size_t x, size_t y, object out) :
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
        
        trap_err("pt_image_tile_file", 
            pt_image_tile_file(self.image, &ti, outf)
        )

    def tile_mem (self, size_t width, size_t height, size_t x, size_t y) :
        cdef pt_tile_info ti
        cdef char *buf
        cdef size_t len

        ti.width = width
        ti.height = height
        ti.x = x
        ti.y = y
        
        # render and return ptr to buffer
        trap_err("pt_image_tile_mem", 
            pt_image_tile_mem(self.image, &ti, &buf, &len)
        )
        
        # copy buffer as str...
        data = python_string.PyString_FromStringAndSize(buf, len)

        # drop buffer...
        stdlib.free(buf)

        return data

    def __dealloc__ (self) :
        if self.image :
            pt_image_destroy(self.image)

