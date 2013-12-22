%module hammer;
%include "exception.i";
%{
#include "allocator.h"
  %}

%ignore HCountedArray_;

%inline %{
  static int h_tt_php;
  %}

%init %{
  h_tt_php = h_allocate_token_type("com.upstandinghackers.hammer.php");
  %}

%inline {
  struct HParsedToken_;
  struct HParseResult_;
  void hpt_to_php(const struct HParsedToken_ *token, zval *return_value);
  
  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data);
 }

%typemap(in) (const uint8_t* str, const size_t len) {
  $1 = (uint8_t*)(*$input)->value.str.val;
  $2 = (*$input)->value.str.len;
 }

%apply (const uint8_t* str, const size_t len) { (const uint8_t* input, size_t length) }
%apply (const uint8_t* str, const size_t len) { (const uint8_t* charset, size_t length) }

%typemap(in) void*[] {
  if (IS_ARRAY == Z_TYPE_PP($input)) {
    HashTable *arr = Z_ARRVAL_PP($input);
    HashPosition pointer;
    int size = zend_hash_num_elements(arr);
    int i = 0;
    int res = 0;
    $1 = (void**)malloc((size+1)*sizeof(HParser*));
    for (i=0; i<size; i++) {
      zval **data;
      if (zend_hash_index_find(arr, i, (void**)&data) == FAILURE) {
	$1 = NULL;
	zend_throw_exception(zend_exception_get_default(TSRMLS_C), "index in parser array out of bounds", 0 TSRMLS_CC);
      } else {
	res = SWIG_ConvertPtr(*data, &($1[i]), SWIGTYPE_p_HParser_, 0 | 0);
	if (!SWIG_IsOK(res)) {
	  zend_throw_exception(zend_exception_get_default(TSRMLS_C), "that wasn't an HParser", 0 TSRMLS_CC);
	}
      }
    } 
  } else {
    $1 = NULL;
    zend_throw_exception(zend_exception_get_default(TSRMLS_C), "that wasn't an array of HParsers", 0 TSRMLS_CC);
  }
 }

%typemap(in) uint8_t {
  if (IS_LONG == Z_TYPE_PP($input)) {
    $1 = Z_LVAL_PP($input);
  } else if (IS_STRING != Z_TYPE_PP($input)) {
    // FIXME raise some error
  } else {
    $1 = *(uint8_t*)Z_STRVAL_PP($input);
  }
 }

%typemap(out) HBytes* {
  RETVAL_STRINGL((char*)$1->token, $1->len, 1);
 }

%typemap(out) struct HParseResult_* {
  if ($1 == NULL) {
    /* If we want parse errors to be exceptions, this is the place to do it */
    //SWIG_exception(SWIG_TypeError, "typemap: should have been an HParseResult*, was NULL");
    RETVAL_NULL();
  } else {
    hpt_to_php($1->ast, $result);
  }
 }

%rename("hammer_token") h_token;
%rename("hammer_int_range") h_int_range;
%rename("hammer_bits") h_bits;
%rename("hammer_int64") h_int64;
%rename("hammer_int32") h_int32;
%rename("hammer_int16") h_int16;
%rename("hammer_int8") h_int8;
%rename("hammer_uint64") h_uint64;
%rename("hammer_uint32") h_uint32;
%rename("hammer_uint16") h_uint16;
%rename("hammer_uint8") h_uint8;
%rename("hammer_whitespace") h_whitespace;
%rename("hammer_left") h_left;
%rename("hammer_right") h_right;
%rename("hammer_middle") h_middle;
%rename("hammer_end") h_end_p;
%rename("hammer_nothing") h_nothing_p;
%rename("hammer_butnot") h_butnot;
%rename("hammer_difference") h_difference;
%rename("hammer_xor") h_xor;
%rename("hammer_many") h_many;
%rename("hammer_many1") h_many1;
%rename("hammer_repeat_n") h_repeat_n;
%rename("hammer_optional") h_optional;
%rename("hammer_ignore") h_ignore;
%rename("hammer_sep_by") h_sepBy;
%rename("hammer_sep_by1") h_sepBy1;
%rename("hammer_epsilon") h_epsilon_p;
%rename("hammer_length_value") h_length_value;
%rename("hammer_and") h_and;
%rename("hammer_not") h_not;
%rename("hammer_indirect") h_indirect;
%rename("hammer_bind_indirect") h_bind_indirect;
%rename("hammer_compile") h_compile;
%rename("hammer_parse") h_parse;

%include "../swig/hammer.i";

%inline {
  void hpt_to_php(const HParsedToken *token, zval *return_value) {
    TSRMLS_FETCH();
    if (!token) {
      RETVAL_NULL();
      return;
    }
    switch (token->token_type) {
    case TT_NONE:
      RETVAL_NULL();
      break;
    case TT_BYTES:
      RETVAL_STRINGL((char*)token->token_data.bytes.token, token->token_data.bytes.len, 1);
      break;
    case TT_SINT:
      RETVAL_LONG(token->token_data.sint);
      break;
    case TT_UINT:
      RETVAL_LONG(token->token_data.uint);
      break;
    case TT_SEQUENCE:
      array_init(return_value);
      for (int i=0; i < token->token_data.seq->used; i++) {
	zval *tmp;
	ALLOC_INIT_ZVAL(tmp);
	hpt_to_php(token->token_data.seq->elements[i], tmp);
	add_next_index_zval(return_value, tmp);
      }
      break;
    default:
      if (token->token_type == h_tt_php) { 
	zval *tmp;
	tmp = (zval*)token->token_data.user;
	RETVAL_ZVAL(tmp, 0, 0);
      } else {
	int res = 0;
	res = SWIG_ConvertPtr(return_value, (void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0);
	if (!SWIG_IsOK(res)) {
	  zend_throw_exception(zend_exception_get_default(TSRMLS_C), "hpt_to_php: that wasn't an HParsedToken", 0 TSRMLS_CC);
	}
	// TODO: support registry
      }
      break;
    }
  }

  static HParsedToken* call_action(const HParseResult *p, void *user_data) {
    zval **args;
    zval func;
    zval *ret;
    TSRMLS_FETCH();
    args = (zval**)h_arena_malloc(p->arena, sizeof(*args) * 1); // one-element array of pointers
    MAKE_STD_ZVAL(args[0]);
    ALLOC_INIT_ZVAL(ret);
    ZVAL_STRING(&func, (const char*)user_data, 0);
    hpt_to_php(p->ast, args[0]);
    int ok = call_user_function(EG(function_table), NULL, &func, ret, 1, args TSRMLS_CC);
    if (ok != SUCCESS) {
      zend_throw_exception(zend_exception_get_default(TSRMLS_C), "call_action failed", 0 TSRMLS_CC);
      return NULL;
    }
    // Whatever the zval is, stuff it into a token
    HParsedToken *tok = h_make(p->arena, h_tt_php, ret);
    return tok;
  }

  static int call_predicate(HParseResult *p, void *user_data) {
    zval **args;
    zval func;
    zval *ret;
    TSRMLS_FETCH();
    args = (zval**)h_arena_malloc(p->arena, sizeof(*args) * 1); // one-element array of pointers
    MAKE_STD_ZVAL(args[0]);
    ALLOC_INIT_ZVAL(ret);
    ZVAL_STRING(&func, (const char*)user_data, 0);
    hpt_to_php(p->ast, args[0]);
    int ok = call_user_function(EG(function_table), NULL, &func, ret, 1, args TSRMLS_CC);
    if (ok != SUCCESS) {
      zend_throw_exception(zend_exception_get_default(TSRMLS_C), "call_predicate failed", 0 TSRMLS_CC);
      return 0;
    }
    return Z_LVAL_P(ret);
  }

  HParser* hammer_action(HParser *parser, const char *name) {
    const char *fname = strdup(name);
    return h_action(parser, call_action, (void*)fname);
  }

  HParser* hammer_predicate(HParser *parser, const char *name) {
    const char *fname = strdup(name);
    return h_attr_bool(parser, call_predicate, (void*)fname);
  }
 }

%pragma(php) code="

function hammer_ch($ch)
{
    if (is_string($ch)) 
        return hammer_token($ch);
    else
        return h_ch($ch);
}

function hammer_choice()
{
    $arg_list = func_get_args();
    $arg_list[] = NULL;
    return h_choice__a($arg_list);    
}

function hammer_sequence()
{
    $arg_list = func_get_args();
    $arg_list[] = NULL;
    return h_sequence__a($arg_list);
}

function hammer_ch_range($low, $high)
{
    return hammer_action(h_ch_range($low, $high), \"chr\");
}

function hammer_in($charset)
{
    return hammer_action(h_in($charset), \"chr\");
}

function hammer_not_in($charset)
{
    return hammer_action(h_not_in($charset), \"chr\");
}
"

