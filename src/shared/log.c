#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/**
 * The global log level
 */
static enum log_level _log_level = LOG_LEVEL_DEFAULT;

/**
 * List of log level names
 */
const char *log_level_names[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
    NULL
};

#define _LOG_LEVEL_NAME(ll) case LOG_ ## ll: return #ll;
const char *log_level_name (enum log_level level)
{
    switch (level) {
        _LOG_LEVEL_NAME(DEBUG)
        _LOG_LEVEL_NAME(INFO)
        _LOG_LEVEL_NAME(WARN)
        _LOG_LEVEL_NAME(ERROR)
        _LOG_LEVEL_NAME(FATAL)
        default: return "???";
    }
}
#undef _LOG_LEVEL_NAME

void set_log_level (enum log_level level)
{
    // meep meep
    _log_level = level;
}

size_t str_append_fmt_va (char *buf_ptr, size_t *buf_size, const char *fmt, va_list args)
{
    int ret;

    if (*buf_size && (ret = vsnprintf(buf_ptr, *buf_size, fmt, args)) < 0)
        return 0;

    if (ret > *buf_size)
        *buf_size = 0;

    else
        *buf_size -= ret;

    return ret;
}

size_t str_append_fmt (char *buf_ptr, size_t *buf_size, const char *fmt, ...)
{
    va_list vargs;
    size_t ret;
    
    va_start(vargs, fmt);
    ret = str_append_fmt_va(buf_ptr, buf_size, fmt, vargs);
    va_end(vargs);

    return ret;
}

void log_output_tag_va (enum log_level level, const char *tag, const char *func, const char *user_fmt, va_list user_fmtargs, const char *log_fmt, va_list log_fmtargs)
{
    char buf[LOG_MSG_MAX], *buf_ptr = buf;
    size_t buf_size = sizeof(buf);

    // filter out?
    if (level < _log_level)
        return;

    buf_ptr += str_append_fmt(buf_ptr, &buf_size, "[%5s] %20s : ", tag, func);

    // output the user data
    if (user_fmt)
        buf_ptr += str_append_fmt_va(buf_ptr, &buf_size, user_fmt, user_fmtargs);
    
    // output the suffix
    if (log_fmt)
        buf_ptr += str_append_fmt_va(buf_ptr, &buf_size, log_fmt, log_fmtargs);

    // display
    // XXX: handle SIGINTR?
    fprintf(stderr, "%s\n", buf);
}

void log_output_tag (enum log_level level, const char *tag, const char *func, const char *user_fmt, va_list user_fmtargs, const char *log_fmt, ...)
{
    va_list vargs;

    va_start(vargs, log_fmt);
    log_output_tag_va(level, tag, func, user_fmt, user_fmtargs, log_fmt, vargs);
    va_end(vargs);
}

void _log_msg (enum log_level level, const char *func, const char *format, ...)
{
    va_list vargs;
    
    // formatted output: no suffix
    va_start(vargs, format);
    log_output_tag(level, log_level_name(level), func, format, vargs, NULL);
    va_end(vargs);
}

void _log_msg_va2 (enum log_level level, const char *func, const char *fmt1, va_list fmtargs1, const char *fmt2, va_list fmtargs2)
{
    log_output_tag_va(level, log_level_name(level), func, fmt1, fmtargs1, fmt2, fmtargs2);
}

void _log_errno (enum log_level level, const char *func, const char *format, ...)
{
    va_list vargs;

    // formatted output: suffix strerror()
    va_start(vargs, format);
    log_output_tag(level, log_level_name(level), func, format, vargs, ": %s", strerror(errno));
    va_end(vargs);
}

void _log_exit (enum log_level level, int exit_code, const char *func, const char *format, ...)
{
    va_list vargs;

    // formatted output without any suffix
    va_start(vargs, format);
    log_output_tag(level, "EXIT", func, format, vargs, NULL);
    va_end(vargs);

    // exit
    exit(exit_code);
}

