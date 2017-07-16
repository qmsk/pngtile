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
		ImageFormat:       ImageFormat(ci.image_format),
		ImageWidth:        uint(ci.image_width),
		ImageHeight:       uint(ci.image_height),
		ImageBPP:          uint(ci.image_bpp),
		ImageModifiedTime: makeTime(ci.image_mtime),
		ImageBytes:        uint(ci.image_bytes),
		CacheVersion:      int(ci.cache_version),
		CacheFormat:       ImageFormat(ci.cache_format),
		CacheModifiedTime: makeTime(ci.cache_mtime),
		CacheBytes:        uint(ci.cache_bytes),
		CacheBlocks:       uint(ci.cache_blocks),
	}
}

type ImageInfo struct {
	ImageFormat       ImageFormat `json:"image_format"`
	ImageWidth        uint        `json:"image_width"`
	ImageHeight       uint        `json:"image_height"`
	ImageBPP          uint        `json:"image_bpp"`
	ImageModifiedTime time.Time   `json:"image_mtime"`
	ImageBytes        uint        `json:"image_bytes"`
	CacheFormat       ImageFormat `json:"cache_format"`
	CacheVersion      int         `json:"cache_version"`
	CacheModifiedTime time.Time   `json:"cache_mtime"`
	CacheBytes        uint        `json:"cache_bytes"`
	CacheBlocks       uint        `json:"cache_blocks"`
}
