#include "util.h"

#include <string.h>
#include <errno.h>

int chfext (char *buf, size_t len, const char *newext)
{
    char *ext;

    // find .ext
    if ((ext = strrchr(buf, '.')) == NULL) {
        errno = EINVAL;
        return -1;
    }

    // check length
    if (ext + strlen(newext) >= buf + len) {
        errno = ENAMETOOLONG;
        return -1;
    }

    // change to .foo
    strcpy(ext, newext);
    
    // ok
    return 0;
}

int path_with_fext (const char *path, char *buf, size_t len, const char *newext)
{
    // verify length
    if (strlen(path) > len) {
        errno = ENAMETOOLONG;
        return -1;
    }

    // copy filename
    strcpy(buf, path);

    // change fext
    return chfext(buf, len, newext);
}
