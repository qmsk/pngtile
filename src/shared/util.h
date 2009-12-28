#ifndef SHARED_UTIL_H
#define SHARED_UTIL_H

#include <stddef.h>

/**
 * Replace the file extension in the given path buffer with the given extension, which should be of the form ".foo"
 */
int chfext (char *path, size_t len, const char *newext);

/**
 * Copy filesystem path with new file extension
 */
int path_with_fext (const char *path, char *buf, size_t len, const char *newext);


#endif
