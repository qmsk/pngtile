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

func makeImageCacheInfo(ci *C.struct_pt_cache_info, ii *C.struct_pt_image_info) ImageInfo {
	return ImageInfo{
		ImageFormat: ImageFormat(ii.format),
		ImageWidth:  uint(ii.width),
		ImageHeight: uint(ii.height),
		ImageBPP:    uint(ii.bpp),

		CacheVersion:      int(ci.version),
		CacheModifiedTime: makeTime(ci.mtime),
		CacheBytes:        uint(ci.bytes),
		CacheBlocks:       uint(ci.blocks),
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
