#include "util.h"

#include <string.h>

int chfext (char *buf, size_t len, const char *newext)
{
    char *ext;

    // find .ext
    if ((ext = strrchr(buf, '.')) == NULL)
        return -1;

    // check length
    if (ext + strlen(newext) >= buf + len)
        return -1;

    // change to .foo
    strcpy(ext, newext);
    
    // ok
    return 0;
}

int path_with_fext (const char *path, char *buf, size_t len, const char *newext)
{
    // verify length
    if (strlen(path) > len)
        return -1;

    // copy filename
    strcpy(buf, path);

    // change fext
    return chfext(buf, len, newext);
}
