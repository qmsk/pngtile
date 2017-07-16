package pngtile

// #cgo CFLAGS: -I${SRCDIR}/../include
// #cgo LDFLAGS: -L${SRCDIR}/../lib -lpngtile
/*
#include "pngtile.h"
*/
import "C"
import "fmt"

type OpenMode int

const (
	OPEN_READ   = C.PT_OPEN_READ
	OPEN_UPDATE = C.PT_OPEN_UPDATE
)

type CacheStatus int

const (
	CACHE_ERROR    = -1
	CACHE_FRESH    = C.PT_CACHE_FRESH
	CACHE_NONE     = C.PT_CACHE_NONE
	CACHE_STALE    = C.PT_CACHE_STALE
	CACHE_INCOMPAT = C.PT_CACHE_INCOMPAT
)

func (cacheStatus CacheStatus) String() string {
	switch cacheStatus {
	case CACHE_ERROR:
		return "error"
	case CACHE_FRESH:
		return "fresh"
	case CACHE_NONE:
		return "none"
	case CACHE_STALE:
		return "stale"
	case CACHE_INCOMPAT:
		return "incompat"
	default:
		return fmt.Sprintf("%d", cacheStatus)
	}
}

func LogDebug(enable bool) {
	C.pt_log_debug = C.bool(enable)
}

func LogWarn(enable bool) {
	C.pt_log_warn = C.bool(enable)
}
