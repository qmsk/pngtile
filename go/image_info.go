package pngtile

/*
#include "pngtile.h"
*/
import "C"
import (
	"time"
)

func makeTime(t C.time_t) time.Time {
	return time.Unix(int64(t), 0)
}

func makeImageInfo(ci *C.struct_pt_image_info) ImageInfo {
	return ImageInfo{
		ImageWidth:        uint(ci.image_width),
		ImageHeight:       uint(ci.image_height),
		ImageBPP:          uint(ci.image_bpp),
		ImageModifiedTime: makeTime(ci.image_mtime),
		ImageBytes:        uint(ci.image_bytes),
		CacheVersion:      int(ci.cache_version),
		CacheModifiedTime: makeTime(ci.cache_mtime),
		CacheBytes:        uint(ci.cache_bytes),
		CacheBlocks:       uint(ci.cache_blocks),
	}
}

type ImageInfo struct {
	ImageWidth, ImageHeight, ImageBPP uint
	ImageModifiedTime                 time.Time
	ImageBytes                        uint
	CacheVersion                      int
	CacheModifiedTime                 time.Time
	CacheBytes                        uint
	CacheBlocks                       uint
}
