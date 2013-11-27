#!/usr/bin/env python

"""
setup.py for Hammer bindings
"""

from distutils.core import setup, Extension

setup(name="hammer",
      version="0.9.0",
      author="Upstanding Hackers, LLC",
      description="""The Hammer parser combinator library""",
      ext_modules=[Extension('_hammer', ['hammer.i'],
                             swig_opts=['-DHAMMER_INTERNAL__NO_STDARG_H', '-I../../'],
                             define_macros = [('SWIG', None)],
                             extra_compile_args = ['-fPIC',
                                                   '-std=gnu99',],
                             include_dirs=['../../'],
                             library_dirs=['../../'],
                             libraries=['hammer'],)],
      py_modules=['hammer'],
)
