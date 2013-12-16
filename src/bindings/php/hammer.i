%module hammer;
%include "exception.i";

%ignore HCountedArray_;

%inline %{
#define PHP_H_TT_PHP_DESCRIPTOR_RES_NAME "Hammer Token"
  static int h_tt_php;
  static int le_h_tt_php_descriptor;
  %}

%init %{
  h_tt_php = h_allocate_token_type("com.upstandinghackers.hammer.php");
  // TODO: implement h_arena_free, register a token dtor here
  le_h_tt_php_descriptor = zend_register_list_destructors_ex(NULL, NULL, PHP_H_TT_PHP_DESCRIPTOR_RES_NAME, module_number);	     
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
	SWIG_exception(SWIG_IndexError, "index in parser array out of bounds");
	$1 = NULL;
      } else {
	res = SWIG_ConvertPtr(*data, &($1[i]), SWIGTYPE_p_HParser_, 0 | 0);
	if (!SWIG_IsOK(res)) {
	  SWIG_exception(SWIG_TypeError, "that wasn't an HParser");
	}
      }
    } 
  } else {
    SWIG_exception(SWIG_TypeError, "that wasn't an array of HParsers");
    $1 = NULL;
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

%include "../swig/hammer.i";

%inline {
  void hpt_to_php(const HParsedToken *token, zval *return_value) {
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
	  SWIG_exception(SWIG_TypeError, "hpt_to_php: that wasn't an HParsedToken");
	}
	// TODO: support registry
      }
      break;
    }
  }

  static HParsedToken* call_action(const HParseResult *p, void *user_data) {
    zval *args[1];
    zval func;
    zval *ret;
    ALLOC_INIT_ZVAL(ret);
    ZVAL_STRING(&func, (const char*)user_data, 0);
    hpt_to_php(p->ast, args[0]);
    int ok = call_user_function(EG(function_table), NULL, &func, ret, 1, args TSRMLS_CC);
    if (ok != SUCCESS) {
      printf("call_user_function failed\n");
      // FIXME throw some error
      return NULL;
    }
    // Whatever the zval is, stuff it into a token
    HParsedToken *tok = h_make(p->arena, h_tt_php, ret);
    return tok;
  }

  HParser* action(HParser *parser, const char *name) {
    return h_action(parser, call_action, (void*)name);
  }
 }

%pragma(php) code="

function ch($ch)
{
    if (is_string($ch)) 
        return h_token($ch);
    else
        return h_ch($ch);
}

function choice()
{
    $arg_list = func_get_args();
    $arg_list[] = NULL;
    return h_choice__a($arg_list);    
}

function sequence()
{
    $arg_list = func_get_args();
    $arg_list[] = NULL;
    return h_sequence__a($arg_list);
}

function in($charset)
{
    return action(h_in($charset), \"chr\");
}

function not_in($charset)
{
    return action(h_not_in($charset), \"chr\");
}
"

