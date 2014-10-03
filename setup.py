from distutils.core import setup
from distutils.extension import Extension

import os.path

try :
    from Cython.Build import cythonize

    CYTHON = True
except ImportError :
    CYTHON = False

if CYTHON and os.path.exists('python/pypngtile.pyx'):
    pypngtile_sources = [ 'python/pypngtile.pyx' ]
elif os.path.exists("python/pypngtile.c"):
    pypngtile_sources = [ 'python/pypngtile.c' ]
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

