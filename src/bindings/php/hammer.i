%module hammer;

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
  static zval* hpt_to_php(const struct HParsedToken_ *token);
  
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
	// FIXME raise some error
	arg1 = NULL;
      } else {
	res = SWIG_ConvertPtr(*data, &(arg1[i]), SWIGTYPE_p_HParser_, 0 | 0);
	if (!SWIG_IsOK(res)) {
	  // TODO do we not *have* SWIG_TypeError?
	  SWIG_exception_fail(res, "that wasn't an HParser");
	}
      }
    } 
  } else {
    // FIXME raise some error
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

/* TODO do we need this anymore? 
%typemap(out) struct HCountedArray_* {

 }
*/
%typemap(out) struct HParseResult_* {
  if ($1 == NULL) {
    // TODO: raise parse failure
    RETVAL_NULL();
  } else {
    if ($1->ast == NULL) {
      RETVAL_NULL();
    } else {
      switch($1->ast->token_type) {
      case TT_NONE:
	RETVAL_NULL();
	break;
      case TT_BYTES:
	RETVAL_STRINGL((char*)$1->ast->token_data.bytes.token, $1->ast->token_data.bytes.len, 1);
	break;
      case TT_SINT:
	RETVAL_LONG($1->ast->token_data.sint);
	break;
      case TT_UINT:
	RETVAL_LONG($1->ast->token_data.uint);
	break;
      case TT_SEQUENCE:
	array_init($result);
	for (int i=0; i < $1->ast->token_data.seq->used; i++) {
	  add_next_index_zval($result, hpt_to_php($1->ast->token_data.seq->elements[i]));
	}
	break;
      default:
	/* if (token->token_type == h_tt_php) { */
	/* 	ZEND_REGISTER_RESOURCE(return_value, token->token_data.user, le_swig__p_void); // it's a void*, what else could I do with it? */
	/* 	return return_value; */
	/* } else { */
	/* 	// I guess that's a python thing */
	/* 	//return (zval*)SWIG_NewPointerObj((void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0); */
	/* 	// TODO: support registry */
	/* } */
	break;
      }
    }
  }
 }
/*
%typemap(in) (HPredicate* pred, void* user_data) {

 }
*/
%typemap(in) (const HAction a, void* user_data) {
  $2 = $input;
  $1 = call_action;
 }

%include "../swig/hammer.i";

%inline {
  static zval* hpt_to_php(const HParsedToken *token) {
    zval *ret;
    ALLOC_INIT_ZVAL(ret);
    if (token == NULL) {
      ZVAL_NULL(ret);
      return ret;
    }
    switch (token->token_type) {
    case TT_NONE:
      ZVAL_NULL(ret);
      return ret;
    case TT_BYTES:
      ZVAL_STRINGL(ret, (char*)token->token_data.bytes.token, token->token_data.bytes.len, 1);
      return ret;
    case TT_SINT:
      ZVAL_LONG(ret, token->token_data.sint);
      return ret;
    case TT_UINT:
      ZVAL_LONG(ret, token->token_data.uint);
      return ret;
    case TT_SEQUENCE:
      array_init(ret);
      for (int i=0; i < token->token_data.seq->used; i++) {
       add_next_index_zval(ret, hpt_to_php(token->token_data.seq->elements[i]));
      }
      return ret;
    default:
      /* if (token->token_type == h_tt_php) { */
      /*       ZEND_REGISTER_RESOURCE(return_value, token->token_data.user, le_swig__p_void); // it's a void*, wh
      /*       return return_value; */
      /* } else { */
      /*       // I guess that's a python thing */
      /*       //return (zval*)SWIG_NewPointerObj((void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0); */
      /*       // TODO: support registry */
      /* } */
      break;
    }
  }

  static struct HParsedToken_* call_action(const struct HParseResult_ *p, void* user_data) {
    zval *args[1];
    zval ret;
    // in PHP land, the HAction is passed by its name as a string
    if (IS_STRING != Z_TYPE_P((zval*)user_data)) {
      // FIXME throw some error
      return NULL;
    }
    zval *callable;
    callable = user_data;
    args[0] = hpt_to_php(p->ast);
    int ok = call_user_function(EG(function_table), NULL, callable, &ret, 1, args TSRMLS_CC);
    if (ok != SUCCESS) {
      // FIXME throw some error
      return NULL;
    }
    // TODO: add reference to ret to parse-local data
    HParsedToken *tok = h_make(p->arena, h_tt_php, &ret);
    return tok;
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

function action($p, $act)
{
    return h_action($p, $act);
}

function in($charset)
{
    return action(h_in($charset), 'chr');
}

function not_in($charset)
{
    return action(h_not_in($charset), 'chr');
}
"

