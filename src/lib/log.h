#ifndef PNGTILE_LOG_H
#define PNGTILE_LOG_H

#include "pngtile.h"

enum {
  PT_LOG_ERRNO = 1,
  PT_LOG_ABORT = 2,
};

void pt_log(bool flags, const char *prefix, const char *func, const char *fmt, ...);

#define PT_DEBUG(...)        do { if (pt_log_debug)  pt_log(0,            "DEBUG libpngtile:",  __func__, __VA_ARGS__); } while(0)
#define PT_WARN_ERRNO(...)   do { if (pt_log_warn)   pt_log(PT_LOG_ERRNO, "WARN  libpngtile:",   __func__, __VA_ARGS__); } while(0)
#define PT_WARN(...)         do { if (pt_log_warn)   pt_log(0,            "WARN  libpngtile:",   __func__, __VA_ARGS__); } while(0)

#define PT_ABORT_ERRNO(...)  do { pt_log(PT_LOG_ERRNO | PT_LOG_ABORT,     "ABORT libpngtile:",  __func__, __VA_ARGS__); } while(0)
#define PT_ABORT(...)        do { pt_log(PT_LOG_ABORT,                    "ABORT libpngtile:",  __func__, __VA_ARGS__); } while(0)

#endif
