#ifndef PNGTILE_ERROR_H
#define PNGTILE_ERROR_H

/**
 * @file
 *
 * Obtuse error handling
 */
#include "pngtile.h"
#include <errno.h>

#define RETURN_ERROR(code) return -(code)
#define RETURN_ERROR_ERRNO(code, errn) do { errno = (errn); return -(code); } while (0)
#define JUMP_ERROR(err) do { goto error; } while (0)
#define JUMP_SET_ERROR(err, code) do { err = -(code); goto error; } while (0)
#define JUMP_SET_ERROR_ERRNO(err, code, errn) do { err = -(code); errno = (errn); goto error; } while (0)

#endif
