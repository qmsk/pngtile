package pngtile

/*
#include <stdlib.h>
#include "pngtile.h"
*/
import "C"
import "unsafe"

// Does this file look like a PNG file that we can use?
func SniffImage(path string) (ImageFormat, bool, error) {
	var c_image_format C.enum_pt_image_format
	var c_path = C.CString(path)
	defer C.free(unsafe.Pointer(c_path))

	if ret, err := C.pt_sniff_image(c_path, &c_image_format); ret < 0 {
		return ImageFormat(ret), false, makeError("pt_sniff_image", ret, err)
	} else {
		return ImageFormat(c_image_format), ret == 0, nil
	}
}

// Convert PNG path to .cache path for use with Image
func CachePath(path string) (string, error) {
	var c_path = C.CString(path)
	var c_buf [1024]C.char

	if ret, err := C.pt_cache_path(c_path, &c_buf[0], 1024); ret < 0 {
		return "", makeError("pt_cache_path", ret, err)
	} else {
		return C.GoString(&c_buf[0]), nil
	}
}

// Danger: must call image.Close() to not leak.
func OpenImage(cachePath string) (*Image, error) {
	return imageOpen(cachePath)
}

// Open image, and pass to func, or return error.
//
// Returns error from func, releasing the image.
func WithImage(cachePath string, f func(*Image) error) error {
	if image, err := imageOpen(cachePath); err != nil {
		return err
	} else {
		defer image.destroy()

		return f(image)
	}
}
