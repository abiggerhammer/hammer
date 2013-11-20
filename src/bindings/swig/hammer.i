%module hammer
%nodefaultctor;

%include "stdint.i"

#if defined(SWIGPHP)
%ignore HCountedArray_;
%typemap(in) uint8_t* {

 }
%typemap(out) uint8_t* {

 }
%typemap(in) void*[] {

 }
%typemap(in) uint8_t {

 }
%typemap(out) HBytes* {

 }
%typemap(out) struct HCountedArray_* {

 }
#else
  #warning no Hammer typemaps defined
#endif

 // All the include paths are relative to the build, i.e., ../../. If you need to build these manually (i.e., not with scons), keep that in mind.
%{
#include "allocator.h"
#include "hammer.h"
#include "internal.h"
%}
%include "allocator.h"
%include "hammer.h"

