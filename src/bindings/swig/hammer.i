%module hammer
%nodefaultctor;

%include "stdint.i"
 //%include "typemaps.i"
 //%apply char [ANY] { uint8_t [ANY] };

#if defined(SWIGPYTHON)
%ignore HCountedArray_;
%typemap(in) uint8_t* {
  Py_INCREF($input);
  $1 = (uint8_t*)PyString_AsString($input);
 }
%typemap(out) uint8_t* {
  $result = PyString_FromString((char*)$1);
 }

%typemap(newfree) HParseResult* {
  h_parse_result_free($1);
 }

%newobject h_parse
%delobject h_parse_result_free

 /*
%typemap(in) (uint8_t* str, size_t len) {
  if (PyString_Check($input) ||
      PyUnicode_Check($input)) {
    PyString_AsStringAndSize($input, (char**)&$1, &$2);
  } else {
    PyErr_SetString(PyExc_TypeError, "Argument must be a str or unicode");
  }    
 }
*/
%apply (char *STRING, size_t LENGTH) {(uint8_t* str, size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* input, size_t length)}
%apply (uint8_t* str, size_t len) {(const uint8_t* str, const size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* charset, size_t length)}
%typemap(in) void*[] {
  if (PyList_Check($input)) {
    Py_INCREF($input);
    int size = PyList_Size($input);
    int i = 0;
    int res = 0;
    $1 = (void**)malloc((size+1)*sizeof(HParser*));
    for (i=0; i<size; i++) {
      PyObject *o = PyList_GetItem($input, i);
      res = SWIG_ConvertPtr(o, &($1[i]), SWIGTYPE_p_HParser_, 0 | 0);
      if (!SWIG_IsOK(res)) {
	SWIG_exception_fail(SWIG_ArgError(res), "that wasn't an HParser" );
      }
    }
    $1[size] = NULL;
  } else {
    PyErr_SetString(PyExc_TypeError, "__a functions take lists of parsers as their argument");
    return NULL;
  }
 }
%typemap(in) uint8_t {
  if (PyInt_Check($input)) {
    $1 = PyInt_AsLong($input);
  }
  else if (!PyString_Check($input)) {
    PyErr_SetString(PyExc_ValueError, "Expecting a string");
    return NULL;
  } else {
    $1 = *(uint8_t*)PyString_AsString($input);
  }
 }
%typemap(out) HBytes* {
  $result = PyString_FromStringAndSize((char*)$1->token, $1->len);
 }
%typemap(out) struct HCountedArray_* {
  int i;
  $result = PyList_New($1->used);
  for (i=0; i<$1->used; i++) {
    HParsedToken *t = $1->elements[i];
    PyObject *o = SWIG_NewPointerObj(SWIG_as_voidptr(t), SWIGTYPE_p_HParsedToken_, 0 | 0);
    PyList_SetItem($result, i, o);
  }
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

