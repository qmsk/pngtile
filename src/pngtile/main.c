#include "pngtile.h"
#include "log.h"

#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

enum option_names {

    _OPT_LONGONLY       = 255,

    OPT_BENCHMARK,
    OPT_RANDOMIZE,
};

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
    { "out",            true,   NULL,   'o' },

    // --long-only options
    { "benchmark",      true,   NULL,   OPT_BENCHMARK   },
    { "randomize",      false,  NULL,   OPT_RANDOMIZE   },
    { 0,                0,      0,      0               }
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
        "\t-h, --help               show this help and exit\n"
        "\t-q, --quiet              supress informational output\n"
        "\t-v, --verbose            display more informational output\n"
        "\t-D, --debug              equivalent to -v\n"
        "\t-U, --force-update       unconditionally update image caches\n"
        "\t-N, --no-update          do not update the image cache\n"
        "\t-B, --background         set background pattern for sparse cache file: 0xHH..\n"
        "\t-W, --width      PX      set tile width\n"
        "\t-H, --height     PX      set tile height\n"
        "\t-x, --x          PX      set tile x offset\n"
        "\t-y, --y          PX      set tile y offset\n"
        "\t-z, --zoom       ZL      set zoom factor (<0)\n"
        "\t-o, --out        FILE    set tile output file\n"
        "\t--benchmark      N       do N tile renders\n"
        "\t--randomize              randomize tile x/y coords\n"
    );
}

unsigned long parse_uint (const char *val, const char *name)
{
    char *endptr;
    long int out;

    // decode
    out = strtol(val, &endptr, 0);

    // validate
    if (*endptr || out < 0)
        EXIT_ERROR(EXIT_FAILURE, "Invalid value for %s: %s", name, val);

    // ok
    return out;
}

signed long parse_sint (const char *val, const char *name)
{
    char *endptr;
    long int out;

    // decode
    out = strtol(val, &endptr, 0);

    // validate
    if (*endptr)
        EXIT_ERROR(EXIT_FAILURE, "Invalid value for %s: %s", name, val);

    // ok
    return out;
}

long randrange (long start, long end)
{
    return start + (rand() * (end - start) / RAND_MAX);
}

/**
 * Randomize tile x/y with given image info
 */
void randomize_tile (struct pt_tile_params *params, const struct pt_image_info *info)
{
    params->x = randrange(0, info->width - params->width);
    params->y = randrange(0, info->height - params->height);
}

/**
 * Render a tile
 */
int do_tile (struct pt_image *image, const struct pt_tile_params *params, const char *out_path)
{
    FILE *out_file = NULL;
    char tmp_name[] = "pt-tile-XXXXXX";
    int err = 0;

    if (!out_path) {
        int fd;

        // temporary file for output
        if ((fd = mkstemp(tmp_name)) < 0) {
            log_errno("mkstemp");
            goto error;
        }

        out_path = tmp_name;

        // open out
        if ((out_file = fdopen(fd, "wb")) == NULL) {
            log_errno("fdopen");
            goto error;
        }

    } else if (strcmp(out_path, "-") == 0) {
        // use stdout
        if ((out_file = fdopen(STDOUT_FILENO, "wb")) == NULL) {
            log_errno("fdopen: STDOUT_FILENO");
            goto error;
        }

    } else {
        // use file
        if ((out_file = fopen(out_path, "wb")) == NULL) {
            log_errno("fopen: %s", out_path);
            goto error;
        }

    }

    // render
    log_info("\tRender tile %ux%u@(%u,%u) -> %s", params->width, params->height, params->x, params->y, out_path);

    if ((err = pt_image_tile_file(image, params, out_file))) {
        log_errno("pt_image_tile_file: %s", pt_strerror(err));
        goto error;
    }

error:
    // cleanup
    if (out_file && fclose(out_file))
        log_warn_errno("fclose: out_file");

    return err;
}

int main (int argc, char **argv)
{
    int opt;
    bool force_update = false, no_update = false, randomize = false;
    struct pt_tile_params params = {
        .width  = 800,
        .height = 600,
        .x      = 0,
        .y      = 0,
        .zoom   = 0
    };
    struct pt_image_params update_params = { };
    const char *out_path = NULL;
    int benchmark = 0;
    int err;

    // parse arguments
    while ((opt = getopt_long(argc, argv, "hqvDUNB:W:H:x:y:z:o:j:", options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                // display help
                help(argv[0]);

                return EXIT_SUCCESS;

            case 'q':
                // supress excess log output
                pt_log_warn = false;
                set_log_level(LOG_WARN);

                break;

            case 'v':
            case 'D':
                // display additional output
                pt_log_debug = true;
                set_log_level(LOG_DEBUG);

                break;

            case 'U':
                // force update of image caches
                force_update = true; break;

            case 'N':
                // supress update of image caches
                no_update = true; break;

            case 'B':
                // background pattern
                {
                    // parse 0xXXXXXXXX
                    if (sscanf(optarg, "0x%02hhx%02hhx%02hhx%02hhx",
                        &update_params.background_pixel[0],
                        &update_params.background_pixel[1],
                        &update_params.background_pixel[2],
                        &update_params.background_pixel[3]
                    ) < 1)
                        FATAL("Invalid hex value for -B/--background: %s", optarg);

                    // store
                    update_params.flags |= PT_IMAGE_BACKGROUND_PIXEL;

                } break;

            case 'W':
                params.width = parse_uint(optarg, "--width"); break;

            case 'H':
                params.height = parse_uint(optarg, "--height"); break;

            case 'x':
                params.x = parse_uint(optarg, "--x"); break;

            case 'y':
                params.y = parse_uint(optarg, "--y"); break;

            case 'z':
                params.zoom = parse_sint(optarg, "--zoom"); break;

            case 'o':
                // output file
                out_path = optarg; break;

            case OPT_BENCHMARK:
                benchmark = parse_uint(optarg, "--benchmark"); break;

            case OPT_RANDOMIZE:
                randomize = true; break;

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


    struct pt_image *image = NULL;
    enum pt_cache_status status;

    // process each image in turn
    log_debug("Processing %d images...", argc - optind);

    for (int i = optind; i < argc; i++) {
        const char *img_path = argv[i];
        char cache_path[1024];

        log_debug("Loading image from: %s...", img_path);

        if ((err = pt_cache_path(img_path, cache_path, sizeof(cache_path)))) {
          log_error("pt_cache_path: %s: %s", img_path, pt_strerror(err));
          goto error;
        }

        // open
        if ((err = pt_image_new(&image, cache_path))) {
            log_errno("pt_image_new: %s: %s", cache_path, pt_strerror(err));
            continue;
        }

        log_info("Opened image from %s at %s", img_path, cache_path);

        // check if stale
        if ((status = pt_image_status(image, img_path)) < 0) {
            log_errno("pt_image_status: %s: %s", img_path, pt_strerror(status));
            goto error;
        }

        // update if stale
        if (status != PT_CACHE_FRESH || force_update) {
            if (status == PT_CACHE_NONE)
                log_info("\tImage cache is missing");

            else if (status == PT_CACHE_STALE)
                log_info("\tImage cache is stale");

            else if (status == PT_CACHE_INCOMPAT)
                log_info("\tImage cache is incompatible");

            else if (status == PT_CACHE_FRESH)
                log_info("\tImage cache is fresh");

            else
                log_warn("\tImage cache status is unknown");

            if (!no_update) {
                log_info("\tUpdating image cache...");

                if ((err = pt_image_update(image, img_path, &update_params))) {
                    log_error("pt_image_update: %s: %s", img_path, pt_strerror(err));
                    goto error;
                }

                log_debug("\tUpdated image cache");

            } else {
                log_warn("\tSupressing cache update");
            }

        } else {
            log_debug("\tImage cache is fresh");

            // ensure it's loaded
            log_info("\tLoad image cache...");

            if ((err = pt_image_open(image))) {
                log_errno("pt_image_open: %s", pt_strerror(err));
                goto error;
            }
        }

        // show info
        struct pt_image_info info;
        struct pt_cache_info cache_info;

        if ((err = pt_image_info(image, &cache_info, &info))) {
            log_warn_errno("pt_image_info: %s", pt_strerror(err));

        } else {
            log_info("\tImage dimensions: %zux%zu (%zu bpp)", info.width, info.height, info.bpp);
            //log_info("\tImage mtime=%ld, bytes=%zu", (long) info.image_mtime, info.image_bytes);
            log_info("\tCache mtime=%ld, bytes=%zu, blocks=%zu (%zu bytes), version=%d",
                    (long) cache_info.mtime, cache_info.bytes, cache_info.blocks, cache_info.blocks * 512, cache_info.version
            );
        }

        // render tile?
        if (benchmark) {
            log_info("\tRunning %d %stile renders...", benchmark, randomize ? "randomized " : "");

            // n times
            for (int i = 0; i < benchmark; i++) {
                // randomize x, y
                if (randomize)
                    randomize_tile(&params, &info);

                if (do_tile(image, &params, out_path))
                    goto error;
            }

        } else if (out_path) {
            // randomize x, y
            if (randomize)
                randomize_tile(&params, &info);

            // just once
            if (do_tile(image, &params, out_path))
                goto error;

        }

        // cleanup
        pt_image_destroy(image);

        continue;

error:
        // quit
        EXIT_ERROR(EXIT_FAILURE, "Processing image failed: %s", img_path);
    }

    log_info("Done");

    return 0;
}
