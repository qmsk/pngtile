#include "path.h"
#include "pngtile.h"

#include <string.h>
#include <errno.h>

int pt_path_make_ext (char *buf, size_t len, const char *path, const char *ext)
{
    char *p;

    if (strlen(path) >= len) {
        return -PT_ERR_PATH;
    }

    strcpy(buf, path);

    // find .ext
    if ((p = strrchr(buf, '.')) == NULL) {
        // TODO: seek to end instead?
        return -PT_ERR_PATH;
    }

    // check length
    if (p + strlen(ext) >= buf + len) {
        return -PT_ERR_PATH;
    }

    // change to .foo
    strcpy(p, ext);

    // ok
    return 0;
}
