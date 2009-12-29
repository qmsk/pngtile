cdef extern from "stdio.h" :
    struct FILE :
        pass

cdef extern from "Python.h" :
    int PyFile_Check (object p)
    FILE* PyFile_AsFile (object p)
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
    int pt_image_tile (pt_image *image, pt_tile_info *info, FILE *out)
    void pt_image_destroy (pt_image *image)

OPEN_UPDATE = PT_OPEN_UPDATE
CACHE_ERROR = PT_CACHE_ERROR
CACHE_FRESH = PT_CACHE_FRESH
CACHE_NONE = PT_CACHE_NONE
CACHE_STALE = PT_CACHE_STALE

class Error (BaseException) :
    pass

cdef class Image :
    cdef pt_image *image

    def __cinit__ (self, char *png_path, int cache_mode = 0) :
        if pt_image_open(&self.image, NULL, png_path, cache_mode) < 0 :
            raise Error("pt_image_open: " + png_path)
    
    def info (self) :
        cdef pt_image_info *image_info

        if pt_image_info_func(self.image, &image_info) < 0 :
            raise Error("pt_image_info")

        return (image_info.width, image_info.height)
    
    def status (self) :
        cdef int status = pt_image_status(self.image)
        
        if status < 0 :
            raise Error("pt_image_status")

        else :
            return status

    def tile (self, size_t width, size_t height, size_t x, size_t y, object out) :
        cdef FILE *outf
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
        
        if pt_image_tile(self.image, &ti, outf) < 0 :
            raise Error("pt_image_tile")

    def __dealloc__ (self) :
        if self.image :
            pt_image_destroy(self.image)

