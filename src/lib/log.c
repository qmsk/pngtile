#include "log.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

bool pt_log_debug = false;
bool pt_log_warn = true;

void pt_log(bool flags, const char *prefix, const char *func, const char *fmt, ...)
{
  va_list vargs;

  fprintf(stderr, "%s%s: ", prefix, func);

  va_start(vargs, fmt);
  vfprintf(stderr, fmt, vargs);
  va_end(vargs);

  if (flags & PT_LOG_ERRNO) {
    fprintf(stderr, ": %s\n", strerror(errno));
  } else {
    fprintf(stderr, "\n");
  }

  if (flags & PT_LOG_ABORT) {
    abort();
  }
}
