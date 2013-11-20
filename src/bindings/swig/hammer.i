%module hammer
%nodefaultctor;

%include "stdint.i"

#if defined(SWIGPHP)
%ignore HCountedArray_;
%typemap(in) (const uint8_t* str, const size_t len) {
  $1 = (uint8_t*)(*$input)->value.str.val;
  $2 = (*$input)->value.str.len;
 }
%typemap(out) (uint8_t* input, size_t length) {
  RETVAL_STRINGL((char*)$1, $2, 1);
 }
%apply (const uint8_t* str, const size_t len) { (const uint8_t* input, size_t length) }
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

