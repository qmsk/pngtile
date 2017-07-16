#ifndef SHARED_UTIL_H
#define SHARED_UTIL_H

#include <stddef.h>

/**
 * Return the ".ext" from the end of path, or a pointer to the end of the string ("") if no ext.
 *
 */
const char *pt_path_ext(const char *path);

/**
 * Copy path to buffer, and replacing given extension.
 * The extension should be of the form ".foo"
 *
 * @param buf build path in buffer
 * @param size size of buffuer
 * @param path original path
 * @param ext file extension to sue
 * @return -PT_ERR_PATH
 */
int pt_path_make_ext (char *buf, size_t size, const char *path, const char *ext);

#endif
