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

    ctypedef pt_image_info* const_image_info_ptr "const struct pt_image_info *"
    
    ## functions
    int pt_image_open (pt_image **image_ptr, pt_ctx *ctx, char *png_path, int cache_mode) nogil
    int pt_image_info_ "pt_image_info" (pt_image *image, pt_image_info **info_ptr) nogil
    int pt_image_status (pt_image *image) nogil
    int pt_image_load (pt_image *image) nogil
    int pt_image_update (pt_image *image, pt_image_params *params) nogil
    int pt_image_tile_file (pt_image *image, pt_tile_info *info, stdio.FILE *out) nogil
    int pt_image_tile_mem (pt_image *image, pt_tile_info *info, char **buf_ptr, size_t *len_ptr) nogil
    void pt_image_destroy (pt_image *image) nogil
    
    # error code -> name
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
    """
        Base class for errors raised by pypngtile.
    """

    def __init__ (self, func, err) :
        super(Error, self).__init__("%s: %s: %s" % (func, pt_strerror(err), strerror(errno)))

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
        cdef int err
        
        # open
        with nogil :
            # XXX: I hope use of path doesn't break...
            err = pt_image_open(&self.image, NULL, path, mode)

        if err :
            raise Error("pt_image_open", err)


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

        cdef const_image_info_ptr infop
        cdef int err

        with nogil :
            err = pt_image_info_(self.image, &infop)

        if err :
            raise Error("pt_image_info", err)
        
        # return as a struct
        return infop[0]


    def status (self) :
        """
            Return a code describing the status of the underlying cache file.

            CACHE_FRESH         - the cache file exists and is up-to-date
            CACHE_NONE          - the cache file does not exist
            CACHE_STALE         - the cache file exists, but is older than the source image
            CACHE_INCOMPAT      - the cache file exists, but is incompatible with this version of the library
        """

        cdef int ret

        with nogil :
            ret = pt_image_status(self.image)

        if ret :
            raise Error("pt_image_status", ret)
        
        return ret

    def open (self) :
        """
            Open the underlying cache file for reading, if available.
        """

        cdef int err

        with nogil :
            err = pt_image_load(self.image)

        if err :
            raise Error("pt_image_load", err)


    def update (self, background_color = None) :
        """
            Update the underlying cache file from the source image.

            background_color    - skip consecutive pixels that match this byte pattern in output

            Requires that the Image was opened using OPEN_UPDATE.
        """

        cdef pt_image_params params
        cdef char *bgcolor
        cdef int err

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
        with nogil :
            err = pt_image_update(self.image, &params)

        if err :
            raise Error("pt_image_update", err)


    def tile_file (self, size_t width, size_t height, size_t x, size_t y, int zoom, object out) :
        """
            Render a region of the source image as a PNG tile to the given output file.

            width       - dimensions of the output tile in px
            height      
            x           - coordinates in the source file
            y
            zoom        - zoom level: out = 2**(-zoom) * in
            out         - output file

            Note that the given file object MUST be a *real* stdio FILE*, not a fake Python object.
        """

        cdef stdio.FILE *outf
        cdef pt_tile_info ti
        cdef int err

        memset(&ti, 0, sizeof(ti))
        
        # convert to FILE
        if not PyFile_Check(out) :
            raise TypeError("out: must be a file object")

        outf = PyFile_AsFile(out)

        if not outf :
            raise TypeError("out: must have a FILE*")
        
        # pack params
        ti.width = width
        ti.height = height
        ti.x = x
        ti.y = y
        ti.zoom = zoom
        
        # render
        with nogil :
            err = pt_image_tile_file(self.image, &ti, outf)

        if err :
            raise Error("pt_image_tile_file", err)


    def tile_mem (self, size_t width, size_t height, size_t x, size_t y, int zoom) :
        """
            Render a region of the source image as a PNG tile, and return the PNG data a a string.

            width       - dimensions of the output tile in px
            height      
            x           - coordinates in the source file
            y
            zoom        - zoom level: out = 2**(-zoom) * in
        """

        cdef pt_tile_info ti
        cdef char *buf
        cdef size_t len
        cdef int err
        
        memset(&ti, 0, sizeof(ti))
        
        # pack params
        ti.width = width
        ti.height = height
        ti.x = x
        ti.y = y
        ti.zoom = zoom
        
        # render and return via buf/len
        with nogil :
            err = pt_image_tile_mem(self.image, &ti, &buf, &len)

        if err :
            raise Error("pt_image_tile_mem", err)
        
        # copy buffer as str...
        data = python_string.PyString_FromStringAndSize(buf, len)

        # drop buffer...
        stdlib.free(buf)

        return data

    # release the pt_image
    def __dealloc__ (self) :
        if self.image :
            pt_image_destroy(self.image)

            self.image = NULL

