#ifndef SHARED_LOG_H
#define SHARED_LOG_H

#include <stdarg.h>

/**
 * Log level definitions
 *
 * XXX: these names conflict with <syslog.h>
 */
enum log_level {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
};

/**
 * List of log_level values as strings, NULL-terminated
 */
extern const char *log_level_names[];

/**
 * The default log level
 */
#define LOG_LEVEL_DEFAULT LOG_INFO

/**
 * Maximum length of a log message, including terminating NUL
 */
#define LOG_MSG_MAX 1024

/**
 * Set the current log level to filter out messages below the given level
 */
void set_log_level (enum log_level level);
/**
 * Internal logging func, meant for using custom level tags. This performs the filtering of log levels.
 *
 * Format the full output line, and pass it to the log_output_func. The line will be of the form:
 *  "[<tag>] <func> : <user_fmt> [ <log_fmt> ]"
 */
void log_output_tag (enum log_level level, const char *tag, const char *func, const char *user_fmt, va_list user_fmtargs, const char *log_fmt, ...)
    __attribute__ ((format (printf, 6, 7)));

/**
 * va_list version of log_output_tag
 */
void log_output_tag_va (enum log_level level, const char *tag, const char *func, const char *user_fmt, va_list user_fmtargs, const char *log_fmt, va_list log_fmtargs);

/**
 * Log a message with the given level
 */
#define log_msg(level, ...) _log_msg(level, __func__, __VA_ARGS__)
void _log_msg (enum log_level level, const char *func, const char *format, ...)
    __attribute__ ((format (printf, 3, 4)));

/**
 * Log a message with the given level with the given format and varargs
 */
#define log_msg_va2(level, fmt1, vargs1, fmt2, vargs2) _log_msg_va(level, __func__, fmt1, vargs1, fmt2, vargs2)
void _log_msg_va2 (enum log_level level, const char *func, const char *fmt1, va_list fmtargs1, const char *fmt2, va_list fmtargs2);


/**
 * Shorthand for log_msg
 */
#define log_debug(...) log_msg(LOG_DEBUG, __VA_ARGS__)
#define log_info(...)  log_msg(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)  log_msg(LOG_WARN,  __VA_ARGS__)
#define log_error(...) log_msg(LOG_ERROR, __VA_ARGS__)
#define log_fatal(...) log_msg(LOG_FATAL, __VA_ARGS__)

/**
 * Log using errno.
 */
#define log_errno(...) _log_errno(LOG_ERROR, __func__, __VA_ARGS__)
#define log_warn_errno(...) _log_errno(LOG_WARN, __func__, __VA_ARGS__)
void _log_errno (enum log_level level, const char *func, const char *format, ...)
    __attribute__ ((format (printf, 3, 4)));

/**
 * Log with an [EXIT] tag at given level, and then exit with given exit code
 */
#define log_exit(level, exit_code, ...) _log_exit(level, exit_code, __func__, __VA_ARGS__)
void _log_exit (enum log_level level, int exit_code, const char *func, const char *format, ...)
    __attribute__ ((format (printf, 4, 5)))
    __attribute__ ((noreturn));

// for abort()
#include <stdlib.h>

/**
 * log_fatal + exit failure
 */
#define FATAL(...) do { log_fatal(__VA_ARGS__); abort(); } while (0)

/**
 * log_perr + exit failure
 */
#define FATAL_ERRNO(...) do { _log_errno(LOG_FATAL, __func__, __VA_ARGS__); abort(); } while (0)

/**
 * Exit with given code, also logging a message at LOG_... with an [EXIT] tag
 */
#define EXIT_INFO(exit_code, ...)  log_exit(LOG_INFO,  exit_code, __VA_ARGS__)
#define EXIT_WARN(exit_code, ...)  log_exit(LOG_WARN,  exit_code, __VA_ARGS__)
#define EXIT_ERROR(exit_code, ...) log_exit(LOG_ERROR, exit_code, __VA_ARGS__)
#define EXIT_FATAL(exit_code, ...) log_exit(LOG_FATAL, exit_code, __VA_ARGS__)

#endif /* LOG_H */
