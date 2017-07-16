package pngtile

/*
#include "pngtile.h"
*/
import "C"

func LogDebug(enable bool) {
	C.pt_log_debug = C.bool(enable)
}

func LogWarn(enable bool) {
	C.pt_log_warn = C.bool(enable)
}
