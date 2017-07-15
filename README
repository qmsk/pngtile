# libpngtile

Constant-time/memory tile-based handling of large PNG images.

## About
    pngtile is a C library (and associated command-line utility) offering efficient random access to partial regions of
    very large PNG images (gigapixel range).

    For this purpose, the library linearly decodes the PNG image to an uncompressed memory-mapped file, which can then
    be later used to encode a portion of this raw pixel data back into a PNG image.

## Notes
    The command-line utility is mainly intended for maintining the image caches and testing, primary usage is expected
    to be performed using the library interface directly. A simple Cython wrapper for a Python extension module is
    provided under python/.

    There is a separate project that provides a web-based tile viewer using Javascript (implemented in Python as a
    WSGI application).

    The .cache files are not portable across different architectures, nor are they compatible across different cache
    format versions.

    The library supports sparse cache files. A pixel-format byte pattern can be provided with --background using
    hexadecimal notation (--background 0xFFFFFF - for 24bpp RGB white), and consecutive regions of that color will
    be omitted in the cache file, which may provide significant gains in space efficiency.

## Build
    The library depends on libpng. The code is developed and tested using:

        * libpng12-dev      png.h       1.2.54-1ubuntu1

    To compile:

        $ make

    The libpngtile.so and pypngtile.so libraries will be placed under lib/, and the 'pngtile' binary under bin/.

## Usage
    Store the .png data files in a directory. You must have write access to the directory when updating the caches,
    which are written as a .cache file alongside the .png file.

    Provide any number of *.png paths as arguments to the ./bin/util command. Each will be opened, and automatically
    updated if the cache doesn't exist yet, or is stale:

        pngtile -v data/*.png

    Use -v/--verbose for more detailed output.


    To render a tile from some image, provide appropriate -W/-H and -x/-y options to pngtile:

        pngtile data/*.png -W 1024 -H 1024 -x 8000 -y 4000

    The output PNG tiles will be written to temporary files, the names of which are shown in the [INFO] output.


    To force-update an image's cache, use the -U/--force-update option:

        pngtile --force-update data/*.png

    Alternatively, to not update an image's cache, use the -N/--no-update option.

## Issues
    At this stage, the library is primarily designed to handle a specific set of PNG images, and hence does not support
    all aspects of the PNG format, nor any other image formats.

    The pt_images opened by main() are not cleaned up before process exit, due to the asynchronous nature of the tile
    render operation's accesses to the underlying pt_cache object.

# pypngtile

Python extension for pngtile

## Dependencies

    * python-cython
    * python-dev

## Development

    $ python setup.py build_ext

## Install
    $ virtualenv /opt/pngtile
    $ make -B install PREFIX=/opt/pngtile
    $ /opt/pngtile/bin/pip install -r requirements.txt
    $ /opt/pngtile/bin/python setup.py build_ext -I /opt/pngtile/include -L /opt/pngtile/lib -R /opt/pngtile/lib
    $ /opt/pngtile/bin/python setup.py install
