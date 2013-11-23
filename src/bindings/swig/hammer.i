%module hammer
%nodefaultctor;
//%nodefaultdtor;

%include "stdint.i"
 //%include "typemaps.i"
 //%apply char [ANY] { uint8_t [ANY] };

#if defined(SWIGPYTHON)
%ignore HCountedArray_;
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
%typemap(out) struct HParseResult_* {
  if ($1 == NULL) {
    // TODO: raise parse failure
    Py_INCREF(Py_None);
    $result = Py_None;
  } else {
    $result = hpt_to_python($1->ast);
  }
 }

%inline %{
  static int h_tt_python;
  %}
%init %{
  h_tt_python = h_allocate_token_type("com.upstandinghackers.hammer.python");
  %}

/*
%typemap(in) (HPredicate* pred, void* user_data) {
  Py_INCREF($input);
  $2 = $input;
  $1 = call_predicate;
 }
*/
%typemap(in) (const HAction a, void* user_data) {
  Py_INCREF($input);
  $2 = $input;
  $1 = call_action;
 }

%inline {
  struct HParsedToken_;
  struct HParseResult_;
  static PyObject* hpt_to_python(const struct HParsedToken_ *token);
  
  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data);
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


%extend HArena_ {
  ~HArena_() {
    h_delete_arena($self);
  }
 };
%extend HParseResult_ {
  ~HParseResult_() {
    h_parse_result_free($self);
  }
};

%newobject h_parse;
%delobject h_parse_result_free;
%newobject h_new_arena;
%delobject h_delete_arena;

#ifdef SWIGPYTHON
%inline {
  static PyObject* hpt_to_python(const HParsedToken *token) {
    // Caller holds a reference to returned object
    PyObject *ret;
    if (token == NULL) {
      Py_RETURN_NONE;
    }
    switch (token->token_type) {
    case TT_NONE:
      Py_RETURN_NONE;
      break;
    case TT_BYTES:
      return PyString_FromStringAndSize((char*)token->token_data.bytes.token, token->token_data.bytes.len);
    case TT_SINT:
      // TODO: return PyINT if appropriate
      return PyLong_FromLong(token->token_data.sint);
    case TT_UINT:
      // TODO: return PyINT if appropriate
      return PyLong_FromUnsignedLong(token->token_data.uint);
    case TT_SEQUENCE:
      ret = PyTuple_New(token->token_data.seq->used);
      for (int i = 0; i < token->token_data.seq->used; i++) {
	PyTuple_SET_ITEM(ret, i, hpt_to_python(token->token_data.seq->elements[i]));
      }
      return ret;
    default:
      if (token->token_type == h_tt_python) {
	ret = (PyObject*)token->token_data.user;
	Py_INCREF(ret);
	return ret;
      } else {
	return SWIG_NewPointerObj((void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0);
	// TODO: support registry
      }
      
    }
  }
  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data) {
    PyObject *callable = user_data;
    PyObject *ret = PyObject_CallFunctionObjArgs(callable,
						 hpt_to_python(p->ast),
						 NULL);
    if (ret == NULL) {
      PyErr_Print();
      assert(ret != NULL);
    }	
    // TODO: add reference to ret to parse-local data
    HParsedToken *tok = h_make(p->arena, h_tt_python, ret);
    return tok;
   }

 }
#endif
