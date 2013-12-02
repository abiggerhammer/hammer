Hammer is a parsing library. Like many modern parsing libraries, it provides a parser combinator interface for writing grammars as inline domain-specific languages, but Hammer also provides a variety of parsing backends. It's also bit-oriented rather than character-oriented, making it ideal for parsing binary data such as images, network packets, audio, and executables.

Hammer is written in C, but will provide bindings for other languages. If you don't see a language you're interested in on the list, just ask.

Hammer currently builds under Linux and OS X. (Windows is coming.)

[![Build Status](https://travis-ci.org/UpstandingHackers/hammer.png)](https://travis-ci.org/UpstandingHackers/hammer)
Features
========
* Bit-oriented -- grammars can include single-bit flags or multi-bit constructs that span character boundaries, with no hassle
* Thread-safe, reentrant
* Benchmarking for parsing backends -- determine empirically which backend will be most time-efficient for your grammar
* Parsing backends:
  * Packrat parsing
  * LL(k) 
  * GLR 
  * LALR
  * Regular expressions 
* Language bindings: 
  * C++ (not yet implemented)
  * Java (not currently building; give us a few days)
  * Python
  * Ruby (not yet implemented)
  * Perl (not yet implemented)
  * [Go](https://github.com/prevoty/hammer)
  * PHP (not yet implemented)
  * .NET (not yet implemented)

Installing
==========
### Prerequisites
* SCons
* a JDK

### Optional Dependencies
* pkg-config (for `make test`)
* glib-2.0 (>= 2.29) (for `make test`)
* glib-2.0-dev (for `make test`)
* swig (for Python bindings)
* python2.7-dev (for Python bindings)

To build, type `scons`. To run the built-in test suite, type `scons test`. For a debug build, add `--variant=debug`

To build bindings, pass a "bindings" argument to scons, e.g. `scons bindings=python`. `scons bindings=python test` will build Python bindings and run tests for both C and Python. `--variant=debug` is valid here too.

For Java, if jni.h and jni_md.h aren't already somewhere on your include path, prepend
`C_INCLUDE_PATH=/path/to/jdk/include` to that.

To make Hammer available system-wide, use `scons install`. This places include files in `/usr/local/include/hammer` 
and library files in `/usr/local/lib` by default; to install elsewhere, add a `prefix=<destination>` argument, e.g. 
`scons install prefix=$HOME`. A suitable `bindings=` argument will install bindings in whatever place your system thinks is appropriate.

Usage
=====
Just `#include <hammer/hammer.h>` (also `#include <hammer/glue.h>` if you plan to use any of the convenience macros) and link with `-lhammer`.

If you've installed Hammer system-wide, you can use `pkg-config` in the usual way.

Examples
========
The `examples/` directory contains some simple examples, currently including:
* base64
* DNS

Known Issues
============
The Python bindings only work with Python 2.7. SCons doesn't work with Python 3, and PyCapsule isn't available in 2.6 and below, so 2.7 is all you get. Sorry about that.

Community
=========
Please join us at `#hammer` on `irc.upstandinghackers.com` if you have any questions or just want to talk about parsing.

Contact
=======
You can also email us at <hammer@upstandinghackers.com>.
