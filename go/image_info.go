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
		ImageFormat:       ImageFormat(ci.format),
		ImageWidth:        uint(ci.width),
		ImageHeight:       uint(ci.height),
		ImageBPP:          uint(ci.bpp),
		CacheVersion:      int(ci.cache.version),
		CacheModifiedTime: makeTime(ci.cache.mtime),
		CacheBytes:        uint(ci.cache.bytes),
		CacheBlocks:       uint(ci.cache.blocks),
	}
}

type ImageInfo struct {
	ImageFormat       ImageFormat `json:"image_format"`
	ImageWidth        uint        `json:"image_width"`
	ImageHeight       uint        `json:"image_height"`
	ImageBPP          uint        `json:"image_bpp"`
	CacheVersion      int         `json:"cache_version"`
	CacheModifiedTime time.Time   `json:"cache_mtime"`
	CacheBytes        uint        `json:"cache_bytes"`
	CacheBlocks       uint        `json:"cache_blocks"`
}
