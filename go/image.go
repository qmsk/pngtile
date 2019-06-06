package pngtile

/*
#include <stdlib.h>
#include "pngtile.h"
*/
import "C"
import "unsafe"

// mode: OPEN_*
func imageOpen(cachePath string) (*Image, error) {
	var image Image

	var c_cache_path = C.CString(cachePath)
	defer C.free(unsafe.Pointer(c_cache_path))

	if ret, err := C.pt_image_new(&image.pt_image, c_cache_path); ret < 0 {
		return nil, makeError("pt_image_new", ret, err)
	}

	return &image, nil
}

type Image struct {
	pt_image *C.struct_pt_image
}

// return: CACHE_*
func (image *Image) Status(path string) (CacheStatus, error) {
	var c_path = C.CString(path)
	defer C.free(unsafe.Pointer(c_path))

	if ret, err := C.pt_image_status(image.pt_image, c_path); ret < 0 {
		return CACHE_ERROR, makeError("pt_image_status", ret, err)
	} else {
		return CacheStatus(ret), nil
	}
}

func (image *Image) Info() (ImageInfo, error) {
	var image_info C.struct_pt_image_info
	var cache_info C.struct_pt_cache_info

	if ret, err := C.pt_image_info(image.pt_image, &cache_info, &image_info); ret < 0 {
		return ImageInfo{}, makeError("pt_image_open", ret, err)
	}

	return makeImageCacheInfo(&cache_info, &image_info), nil
}

// Open image cache in read-only mode
func (image *Image) Open() error {
	if ret, err := C.pt_image_open(image.pt_image); ret < 0 {
		return makeError("pt_image_open", ret, err)
	}

	return nil
}

// Open image cache in update mode,
func (image *Image) Update(path string, params ImageParams) error {
	var image_params = params.c_struct()
	var c_path = C.CString(path)
	defer C.free(unsafe.Pointer(c_path))

	if ret, err := C.pt_image_update(image.pt_image, c_path, &image_params); ret < 0 {
		return makeError("pt_image_update", ret, err)
	}

	return nil
}

// Render tile to PNG image
func (image *Image) Tile(params TileParams) ([]byte, error) {
	var tile_params = params.c_struct()
	var tile_buf *C.char
	var tile_size C.size_t

	if ret, err := C.pt_image_tile_mem(image.pt_image, &tile_params, &tile_buf, &tile_size); ret < 0 {
		return nil, makeError("pt_image_tile_mem", ret, err)
	} else {
		defer C.free(unsafe.Pointer(tile_buf))
	}

	return C.GoBytes(unsafe.Pointer(tile_buf), C.int(tile_size)), nil // XXX: overflow?
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
