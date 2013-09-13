Hammer is a parsing library. Like many modern parsing libraries, it provides a parser combinator interface for writing grammars as inline domain-specific languages, but Hammer also provides a variety of parsing backends. It's also bit-oriented rather than character-oriented, making it ideal for parsing binary data such as images, network packets, audio, and executables.

Hammer is written in C, but will provide bindings for other languages. If you don't see a language you're interested in on the list, just ask.

Hammer currently builds under Linux. (Windows and OSX are coming.)

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
  * Java
  * Python (not yet implemented)
  * Ruby (not yet implemented)
  * Perl (not yet implemented)
  * Go (not yet implemented)
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

To build, type `scons`. To run the built-in test suite, type `scons test`. For a debug build, add `--variant=debug`

If jni.h and jni_md.h aren't already somewhere on your include path, prepend
`C_INCLUDE_PATH=/path/to/jdk/include` to that.

There is currently no `install` target; to make Hammer available system-wide,
copy `libhammer.a` and `libhammer.so` from `build/opt/src` to `/usr/lib/` (or
`/usr/local/lib/`, or wherever ld will find it) and `hammer.h` to
`/usr/include/`. 

Usage
=====
Just `#include <hammer.h>` and link with `-lhammer`.

Examples
========
The `examples/` directory contains some simple examples, currently including:
* base64
* DNS

Community
=========
Please join us at `#hammer` on `irc.upstandinghackers.com` if you have any questions or just want to talk about parsing.

Contact
=======
You can also email us at <hammer@upstandinghackers.com>.
