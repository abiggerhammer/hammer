#!/usr/bin/env python

"""
setup.py for Hammer bindings
"""
import os.path, sys
from distutils.core import setup, Extension

setup(name="hammer",
      version="0.9.0",
      author="Upstanding Hackers, LLC",
      author_email="hammer@upstandinghackers.com",
      url="https://github.com/UpstandingHackers/hammer",
      description="""The Hammer parser combinator library""",
      ext_modules=[Extension('_hammer', [os.path.join(os.path.dirname(sys.argv[0]),
                                                      'hammer.i')],
                             # swig_opts is set by SConscript
                             define_macros=[('SWIG', None)],
                             depends=['allocator.h', 
                                      'glue.h', 
                                      'hammer.h',
                                      'internal.h',],
                             extra_compile_args=['-fPIC',
                                                 '-std=gnu99',],
                             include_dirs=['src'],
                             library_dirs=['src'],
                             libraries=['hammer'],)],
      
      py_modules=['hammer'],
)
