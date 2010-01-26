from distutils.core import setup
from distutils.extension import Extension

import os.path

build_root = os.path.abspath(os.path.dirname(__file__))

pypngtile_c = "python/pypngtile.c"
pypngtile_name = "python/pypngtile.pyx"

cmdclass = dict()

try :
    from Cython.Distutils import build_ext
    
    cmdclass['build_ext'] = build_ext

except ImportError :
    path = os.path.join(build_root, pypngtile_c)

    if os.path.exists(path) :
        print "Warning: falling back from .pyx -> .c due to missing Cython"
        # just use the .c
        pypngtile_name = pypngtile_c
    
    else :
        # fail
        raise

setup(
    name = 'pngtiles',
    cmdclass = cmdclass,
    ext_modules = [
        Extension("pypngtile", [pypngtile_name],
            include_dirs = ['include'],
            library_dirs = ['lib'],
            libraries = ['pngtile'],
        ),
    ],
)

