package pngtile

/*
#include <stdlib.h>
#include "pngtile.h"
*/
import "C"
import "unsafe"

// mode: OPEN_*
func imageOpen(path string, mode OpenMode) (*Image, error) {
	var image Image

	var c_path = C.CString(path)
	defer C.free(unsafe.Pointer(c_path))
	var c_mode = C.int(mode)

	if ret, err := C.pt_image_new(&image.pt_image, c_path, c_mode); ret < 0 {
		return nil, makeError("pt_image_new", ret, err)
	}

	return &image, nil
}

type Image struct {
	pt_image *C.struct_pt_image
}

// return: CACHE_*
func (image *Image) Status() (CacheStatus, error) {
	if ret, err := C.pt_image_status(image.pt_image); ret < 0 {
		return CACHE_ERROR, makeError("pt_image_status", ret, err)
	} else {
		return CacheStatus(ret), nil
	}
}

// Open image cache in read-only mode
func (image *Image) Open() error {
	if ret, err := C.pt_image_open(image.pt_image); ret < 0 {
		return makeError("pt_image_open", ret, err)
	}

	return nil
}

// Open image cache in update mode,
func (image *Image) Update() error {
	if ret, err := C.pt_image_update(image.pt_image, nil); ret < 0 {
		return makeError("pt_image_open", ret, err)
	}

	return nil
}

func (image *Image) Info() (ImageInfo, error) {
	var image_info C.struct_pt_image_info

	if ret, err := C.pt_image_info(image.pt_image, &image_info); ret < 0 {
		return ImageInfo{}, makeError("pt_image_open", ret, err)
	}

	return makeImageInfo(&image_info), nil
}

// Close image, and destroy it to release resources.
// The Image is no longer usable, even after error returns.
func (image *Image) Close() error {
	if image.pt_image == nil {
		return nil // XXX: error?
	}

	defer image.destroy()

	if ret, err := C.pt_image_close(image.pt_image); ret < 0 {
		return makeError("pt_image_close", ret, err)
	}

	return nil
}

// Abort if open, release.
// Safe to re-call.
func (image *Image) destroy() {
	if image.pt_image != nil {
		C.pt_image_destroy(image.pt_image)
		image.pt_image = nil
	}
}
