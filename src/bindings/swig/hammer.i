%module hammer

%include "stdint.i"
%include "typemaps.i"
%apply char [ANY] { uint8_t [ANY] };

#if defined(SWIGPYTHON)
%typemap(in) uint8_t* {
  $1 = (uint8_t*)PyString_AsString($input);
 }
%typemap(out) uint8_t* {
  $result = PyString_FromString((char*)$1);
 }
#else
  #warning no uint8_t* typemaps defined
#endif

 // All the include paths are relative to the build, i.e., ../../. If you need to build these manually (i.e., not with scons), keep that in mind.
%{
#include "allocator.h"
#include "hammer.h"
#include "internal.h"
%}
%include "allocator.h"
%include "hammer.h"



