from distutils.core import setup
from distutils.extension import Extension

import os.path

try :
    from Cython.Build import cythonize

    CYTHON = True
except ImportError :
    CYTHON = False

if CYTHON and os.path.exists('pypngtile.pyx'):
    pypngtile_sources = [ 'pypngtile.pyx' ]
elif os.path.exists("pypngtile.c"):
    pypngtile_sources = [ 'pypngtile.c' ]
else:
    raise Exception("Building from source requires Cython")

ext_modules = [Extension("pypngtile",
    sources         = pypngtile_sources,
    libraries       = ['pngtile'],
)]

if CYTHON:
    ext_modules = cythonize(ext_modules)

setup(
    name            = 'pngtile',
    version         = '1.0-dev',

    packages        = [ 'pngtile' ],
    ext_modules     = ext_modules,
    scripts         = [
        'bin/pypngtile',
        'bin/tile-server',
    ],
)
