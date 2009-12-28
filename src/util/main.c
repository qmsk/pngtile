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
    );
}

int main (int argc, char **argv)
{
    int opt;
    bool force_update = false;
    
    // parse arguments
    while ((opt = getopt_long(argc, argv, "hqvDU", options, NULL)) != -1) {
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
    int stale;

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
        if ((stale = pt_image_stale(image)) < 0) {
            log_errno("pt_image_stale: %s", img_path);
            goto error;
        }
        
        // update if stale
        if (stale || force_update) {
            if (stale)
                log_debug("Image cache is stale, updating...");
            else // force_update
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

        // done

error:
        // cleanup
        pt_image_destroy(image);
    }

    // XXX: done
    return 0;
}

