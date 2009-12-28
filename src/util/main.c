#include "lib/pngtile.h"
#include "shared/log.h"

#include <getopt.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * Command-line options
 */
static const struct option options[] = {
    { "help",           false,  NULL,   'h' },
    { "quiet",          false,  NULL,   'q' },
    { "verbose",        false,  NULL,   'v' },
    { "debug",          false,  NULL,   'D' },
    { "force-update",   false,  NULL,   'U' },
    { "width",          true,   NULL,   'W' },
    { "height",         true,   NULL,   'H' },
    { "x",              true,   NULL,   'x' },
    { "y",              true,   NULL,   'y' },
    { 0,                0,      0,      0   }
};

/**
 * Print usage/help info on stderr
 */
void help (const char *argv0)
{
    fprintf(stderr, "Usage: %s [options] <image> [...]\n", argv0);
    fprintf(stderr,
        "XXX: Process some image files.\n"
        "\n"
        "\t-h, --help           show this help and exit\n"
        "\t-q, --quiet          supress informational output\n"
        "\t-v, --verbose        display more informational output\n"
        "\t-D, --debug          equivalent to -v\n"
        "\t-U, --force-update   unconditionally update image caches\n"
        "\t-W, --width          set tile width\n"
        "\t-H, --height         set tile height\n"
        "\t-x, --x              set tile x offset\n"
        "\t-y, --y              set tile z offset\n"
    );
}

int main (int argc, char **argv)
{
    int opt;
    bool force_update = false;
    struct pt_tile_info ti = {0, 0, 0, 0};
    
    // parse arguments
    while ((opt = getopt_long(argc, argv, "hqvDUW:H:x:y:", options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                // display help
                help(argv[0]);
                
                return EXIT_SUCCESS;

            case 'q':
                // supress excess log output
                set_log_level(LOG_WARN);

                break;

            case 'v':
            case 'D':
                // display additional output
                set_log_level(LOG_DEBUG);

                break;
            
            case 'U':
                // force update of image caches
                force_update = true;
                
                break;

            case 'W':
                ti.width = strtol(optarg, NULL, 0); break;

            case 'H':
                ti.height = strtol(optarg, NULL, 0); break;

            case 'x':
                ti.x = strtol(optarg, NULL, 0); break;

            case 'y':
                ti.y = strtol(optarg, NULL, 0); break;

            case '?':
                // useage error
                help(argv[0]);

                return EXIT_FAILURE;

            default:
                // getopt???
                FATAL("getopt_long returned unknown code %d", opt);
        }
    }
    
    // end-of-arguments?
    if (!argv[optind])
        EXIT_WARN(EXIT_FAILURE, "No images given");
    


    struct pt_ctx *ctx = NULL;
    struct pt_image *image = NULL;
    enum pt_cache_status status;

    log_debug("Processing %d images...", argc - optind);

    for (int i = optind; i < argc; i++) {
        const char *img_path = argv[i];

        log_debug("Loading image from: %s...", img_path);

        // open
        if (pt_image_open(&image, ctx, img_path, PT_IMG_WRITE)) {
            log_errno("pt_image_open: %s", img_path);
            continue;
        }

        log_info("Opened image at: %s", img_path);
        
        // check if stale
        if ((status = pt_image_status(image)) < 0) {
            log_errno("pt_image_status: %s", img_path);
            goto error;
        }
        
        // update if stale
        if (status != PT_CACHE_FRESH || force_update) {
            if (status == PT_CACHE_NONE)
                log_debug("Image cache is missing");

            else if (status == PT_CACHE_STALE)
                log_debug("Image cache is stale");

            else if (status == PT_CACHE_FRESH)
                log_debug("Image cache is fresh");

            log_debug("Updating image cache...");

            if (pt_image_update(image)) {
                log_warn_errno("pt_image_update: %s", img_path);
            }

            log_info("Updated image cache");
        }

        // show info
        const struct pt_image_info *img_info;
        
        if (pt_image_info(image, &img_info))
            log_warn_errno("pt_image_info: %s", img_path);

        else
            log_info("\tImage dimensions: %zux%zu", img_info->width, img_info->height);

        // render tile?
        if (ti.width && ti.height) {
            log_debug("Render tile %zux%zu@(%zu,%zu) -> stdout", ti.width, ti.height, ti.x, ti.y);

            if (pt_image_tile(image, &ti, stdout))
                log_errno("pt_image_tile: %s", img_path);
        }

error:
        // cleanup
        pt_image_destroy(image);
    }

    // XXX: done
    return 0;
}
