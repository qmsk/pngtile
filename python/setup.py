from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

setup(
    name = 'pngtiles',
    cmdclass = {'build_ext': build_ext},
    ext_modules = [
        Extension("pypngtile", ["pngtile.pyx"],
            include_dirs = ['../include'],
            library_dirs = ['../lib'],
            libraries = ['pngtile'],
        ),
    ],
)

