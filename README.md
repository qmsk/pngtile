# libpngtile

Constant-time/memory handling of large PNG images as smaller tiles.

Language support:

* [Python](python/)
* [Go](go/)

## Docker

If you have some `.png` files in your current directory:

    $ docker pull qmsk/pngtile
    $ docker run --rm -v $PWD:/srv/pngtile/images -p 9090:9090 qmsk/pngtile

The default image `CMD` will build the cache files, and then run the server on HTTP port 9090.

## About
pngtile is a C library (and associated command-line utility) offering efficient random access to partial regions of
very large PNG images (gigapixel range).

For this purpose, the library linearly decodes the PNG image to an uncompressed memory-mapped file, which can then
be later used to encode a portion of this raw pixel data back into a PNG image.

## Notes
The command-line utility is mainly intended for maintining the image caches and testing, primary usage is expected
to be performed using the library interface directly.

A simple Cython wrapper for a Python extension module is provided under `python/`. A CGo wrapper is provided under `go/`.

There is a separate project that provides a web-based tile viewer using Javascript (implemented in Python as a
WSGI application).

The `.cache` files are not portable across different architectures, nor are they compatible across different cache
format versions.

The library supports sparse cache files. A pixel-format byte pattern can be provided with --background using
hexadecimal notation (`--background 0xFFFFFF` - for 24bpp RGB white), and consecutive regions of that color will
be omitted in the cache file, which may provide significant gains in space efficiency.

## Build

The library depends on `libpng`. The code is developed and tested using:

* `libpng12-dev` `png.h` (Ubuntu xenial `1.2.54-1ubuntu1`)

To compile:

    $ make

The `libpngtile.so` and `pypngtile.so` libraries will be placed under `lib/`, and the `pngtile` binary under `bin/`.

Alternatively, use the included `Dockerfile` to build:

    $ docker build -t pngtile .

However, this is slower than desired for development purposes.

## Usage

```
Usage: ./bin/pngtile [options] <image> [...]
Open each of the given image files, check cache status, optionally update their cache, display image info, and
optionally render a tile of each.

        -h, --help               show this help and exit
        -q, --quiet              supress informational output
        -v, --verbose            display more informational output
        -D, --debug              equivalent to -v
        -U, --force-update       unconditionally update image caches
        -N, --no-update          do not update the image cache
        -B, --background         set background pattern for sparse cache file: 0xHH..
        -W, --width      PX      set tile width
        -H, --height     PX      set tile height
        -x, --x          PX      set tile x offset
        -y, --y          PX      set tile y offset
        -z, --zoom       ZL      set zoom factor (<0)
        -o, --out        FILE    set tile output file
        --benchmark      N       do N tile renders
        --randomize              randomize tile x/y coords
```


Provide any number of `*.png` paths as arguments to the `./bin/pngtile` command. Each will be opened, and automatically
updated if the cache doesn't exist yet, or is stale:

    pngtile -v data/*.png

You must have write access to the directory when updating the caches, which are written as a .cache file alongside the .png file.

Use `-v/--verbose` for more detailed output.


To render a tile from some image, provide appropriate `-W/-H` and `-x/-y` options to pngtile:

    pngtile data/*.png -W 1024 -H 1024 -x 8000 -y 4000

The output PNG tiles will be written to temporary files, the names of which are shown in the [INFO] output.

To force-update an image's cache, use the `-U/--force-update` option:

    pngtile --force-update data/*.png

Alternatively, to not update an image's cache, use the `-N/--no-update` option.

## Issues
At this stage, the library is primarily designed to handle a specific set of PNG images, and hence does not support
all aspects of the PNG format, nor any other image formats.
