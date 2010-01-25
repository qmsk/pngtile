#include "shared/log.h"

#include "pngtile.h"

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
    { "no-update",      false,  NULL,   'N' },
    { "background",     true,   NULL,   'B' },
    { "width",          true,   NULL,   'W' },
    { "height",         true,   NULL,   'H' },
    { "x",              true,   NULL,   'x' },
    { "y",              true,   NULL,   'y' },
    { "zoom",           true,   NULL,   'z' },
    { "threads",        true,   NULL,   'j' },
    { 0,                0,      0,      0   }
};

/**
 * Print usage/help info on stderr
 */
void help (const char *argv0)
{
    fprintf(stderr, "Usage: %s [options] <image> [...]\n", argv0);
    fprintf(stderr,
        "Open each of the given image files, check cache status, optionally update their cache, display image info, and\n"
        "optionally render a tile of each.\n"
        "\n"
        "\t-h, --help           show this help and exit\n"
        "\t-q, --quiet          supress informational output\n"
        "\t-v, --verbose        display more informational output\n"
        "\t-D, --debug          equivalent to -v\n"
        "\t-U, --force-update   unconditionally update image caches\n"
        "\t-N, --no-update      do not update the image cache\n"
        "\t-B, --background     set background pattern for sparse cache file: 0xHH..\n"
        "\t-W, --width          set tile width\n"
        "\t-H, --height         set tile height\n"
        "\t-x, --x              set tile x offset\n"
        "\t-y, --y              set tile y offset\n"
        "\t-z, --zoom           set zoom factor (<0)\n"
        "\t-j, --threads        number of threads\n"
    );
}

int main (int argc, char **argv)
{
    int opt;
    bool force_update = false, no_update = false;
    struct pt_tile_info ti = {0, 0, 0, 0, 0};
    struct pt_image_params update_params = { };
    int threads = 2;
    int tmp, err;
    
    // parse arguments
    while ((opt = getopt_long(argc, argv, "hqvDUNB:W:H:x:y:z:j:", options, NULL)) != -1) {
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
            
            case 'N':
                // supress update of image caches
                no_update = true;

                break;

            case 'B':
                // background pattern
                {
                    unsigned int b1 = 0, b2 = 0, b3 = 0, b4 = 0;
                    
                    // parse 0xXXXXXXXX
                    if (sscanf(optarg, "0x%02x%02x%02x%02x", &b1, &b2, &b3, &b4) < 1)
                        FATAL("Invalid hex value for -B/--background: %s", optarg);
                    
                    // store
                    update_params.background_color[0] = b1;
                    update_params.background_color[1] = b2;
                    update_params.background_color[2] = b3;
                    update_params.background_color[3] = b4;

                } break;

            case 'W':
                ti.width = strtol(optarg, NULL, 0); break;

            case 'H':
                ti.height = strtol(optarg, NULL, 0); break;

            case 'x':
                ti.x = strtol(optarg, NULL, 0); break;

            case 'y':
                ti.y = strtol(optarg, NULL, 0); break;

            case 'z':
                ti.zoom = strtol(optarg, NULL, 0); break;

            case 'j':
                if ((tmp = strtol(optarg, NULL, 0)) < 1)
                    FATAL("Invalid value for -j/--threads: %s", optarg);

                threads = tmp; break;

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

    // build ctx
    log_debug("Construct pt_ctx with %d threads", threads);

    if ((err = pt_ctx_new(&ctx, threads)))
        EXIT_ERROR(EXIT_FAILURE, "pt_ctx_new: threads=%d", threads);

    
    // process each image in turn
    log_debug("Processing %d images...", argc - optind);

    for (int i = optind; i < argc; i++) {
        const char *img_path = argv[i];

        log_debug("Loading image from: %s...", img_path);

        // open
        if ((err = pt_image_open(&image, ctx, img_path, PT_OPEN_UPDATE))) {
            log_errno("pt_image_open: %s: %s", img_path, pt_strerror(err));
            continue;
        }

        log_info("Opened image at: %s", img_path);
        
        // check if stale
        if ((status = pt_image_status(image)) < 0) {
            log_errno("pt_image_status: %s: %s", img_path, pt_strerror(status));
            goto error;
        }
        
        // update if stale
        if (status != PT_CACHE_FRESH || force_update) {
            if (status == PT_CACHE_NONE)
                log_debug("\tImage cache is missing");

            else if (status == PT_CACHE_STALE)
                log_debug("\tImage cache is stale");
            
            else if (status == PT_CACHE_INCOMPAT)
                log_debug("\tImage cache is incompatible");

            else if (status == PT_CACHE_FRESH)
                log_debug("\tImage cache is fresh");

            else
                log_warn("\tImage cache status is unknown");
            
            if (!no_update) {
                log_debug("\tUpdating image cache...");

                if ((err = pt_image_update(image, &update_params))) {
                    log_warn_errno("pt_image_update: %s: %s", img_path, pt_strerror(err));
                }

                log_info("\tUpdated image cache");

            } else {
                log_warn("\tSupressing cache update");
            }

        } else {    
            log_debug("\tImage cache is fresh");
        }

        // show info
        const struct pt_image_info *info;
        
        if ((err = pt_image_info(image, &info))) {
            log_warn_errno("pt_image_info: %s: %s", img_path, pt_strerror(err));

        } else {
            log_info("\tImage dimensions: %zux%zu (%zu bpp)", info->img_width, info->img_height, info->img_bpp);
            log_info("\tImage mtime=%ld, bytes=%zu", (long) info->image_mtime, info->image_bytes);
            log_info("\tCache mtime=%ld, bytes=%zu, blocks=%zu (%zu bytes), version=%d", 
                    (long) info->cache_mtime, info->cache_bytes, info->cache_blocks, info->cache_blocks * 512, info->cache_version
            );
        }

        // render tile?
        if (ti.width && ti.height) {
            char tmp_name[] = "pt-tile-XXXXXX";
            int fd;
            FILE *out;
            
            // temporary file for output
            if ((fd = mkstemp(tmp_name)) < 0) {
                log_errno("mkstemp");
                
                continue;
            }

            // open out
            if ((out = fdopen(fd, "w")) == NULL) {
                log_errno("fdopen");

                continue;
            }
            
            // ensure it's loaded
            log_debug("\tLoad image cache...");

            if ((err = pt_image_load(image)))
                log_errno("pt_image_load: %s", pt_strerror(err));

            // render
            log_info("\tAsync render tile %zux%zu@(%zu,%zu) -> %s", ti.width, ti.height, ti.x, ti.y, tmp_name);


            if ((err = pt_image_tile_async(image, &ti, out)))
                log_errno("pt_image_tile: %s: %s", img_path, pt_strerror(err));
        }

error:
        // cleanup
        // XXX: leak because of async: pt_image_destroy(image);
        ;
    }

    log_info("Waiting for images to finish...");

    // wait for tile operations to finish...
    pt_ctx_shutdown(ctx);

    log_info("Done");

    return 0;
}

