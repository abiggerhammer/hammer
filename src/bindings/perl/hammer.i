%module hammer;
%begin %{
#include <unistd.h>
#include <stdint.h>
%}

%inline %{
  static int h_tt_perl;
  %}
%init %{
  h_tt_perl = h_allocate_token_type("com.upstandinghackers.hammer.perl");
  %}


%apply (char *STRING, size_t LENGTH) {(uint8_t* str, size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* input, size_t length)}
%apply (uint8_t* str, size_t len) {(const uint8_t* str, const size_t len)}
%apply (uint8_t* str, size_t len) {(const uint8_t* charset, size_t length)}

%typemap(out) struct HParseResult_* {
  SV* hpt_to_perl(const struct HParsedToken_ *token);
  if ($1 == NULL) {
    // TODO: raise parse failure
    $result = newSV(0);
  } else {
    $result = hpt_to_perl($1->ast);
    //hpt_to_perl($1->ast);
  }
 }

%typemap(in) uint8_t {
  if (SvIOKp($input)) {
    $1 = SvIV($input);
  } else if (SvPOKp($input)) {
    IV len;
    uint8_t* ival = SvPV($input, len);
    if (len < 1) {
      %type_error("Expected string with at least one character");
      SWIG_fail;
    }
    $1 = ival[0];
  } else {
    %type_error("Expected int or string");
    SWIG_fail;
  }
 }


%typemap(newfree) struct HParseResult_* {
  h_parse_result_free($input);
 }

%rename("token") h_token;
%rename("%(regex:/^h_(.*)/\\1/)s", regextarget=1) "^h_u?int(64|32|16|8)";
%rename("int_range") h_int_range;
%include "../swig/hammer.i";
  

%{
  SV* hpt_to_perl(const HParsedToken *token) {
    // All values that this function returns have a refcount of exactly 1.
    SV *ret;
    if (token == NULL) {
      return newSV(0); // TODO: croak.
    }
    switch (token->token_type) {
    case TT_NONE:
      return newSV(0);
      break;
    case TT_BYTES:
      return newSVpvn((char*)token->token_data.bytes.token, token->token_data.bytes.len);
    case TT_SINT:
      // TODO: return PyINT if appropriate
      return newSViv(token->token_data.sint);
    case TT_UINT:
      // TODO: return PyINT if appropriate
      return newSVuv(token->token_data.uint);
    case TT_SEQUENCE: {
      AV* aret;
      av_extend(aret, token->token_data.seq->used);
      for (int i = 0; i < token->token_data.seq->used; i++) {
	av_store(aret, i, hpt_to_perl(token->token_data.seq->elements[i]));
      }
      return newRV_noinc((SV*)aret);
    }
    default:
      if (token->token_type == h_tt_perl) {
	return  (SV*)token->token_data.user;
	
	return SvREFCNT_inc(ret);
      } else {
	return SWIG_NewPointerObj((void*)token, SWIGTYPE_p_HParsedToken_, 0 | 0);
	// TODO: support registry
      }
      
    }
  
  }
  /*
  HParser* ch(uint8_t chr) {
    return h_action(h_ch(chr), h__to_dual_char, NULL);
  }
  HParser* in(const uint8_t *charset, size_t length) {
    return h_action(h_in(charset, length), h__to_dual_char, NULL);
  }
 HParser* not_in(const uint8_t *charset, size_t length) {
   return h_action(h_not_in(charset, length), h__to_dual_char, NULL);
 }
  */
 HParsedToken* h__to_char(const HParseResult* result, void* user_data) {
    assert(result != NULL);
    assert(result->ast != NULL);
    assert(result->ast->token_type == TT_UINT);
    
    uint8_t buf = result->ast->token_data.uint;
    SV *sv = newSVpvn(&buf, 1);
    // This was a failed experiment; for now, you'll have to use ord yourself.
    //sv_setuv(sv, buf);
    //SvPOK_on(sv);
      
    HParsedToken *res = h_arena_malloc(result->arena, sizeof(HParsedToken));
    res->token_type = h_tt_perl;
    res->token_data.user = sv;
    return res;
  }

%}
%inline {
  HParser* ch(uint8_t chr) {
    return h_action(h_ch(chr), h__to_char, NULL);
  }
  HParser* ch_range(uint8_t c0, uint8_t c1) {
    return h_action(h_ch_range(c0,c1), h__to_char, NULL);
  }
  HParser* in(const uint8_t *charset, size_t length) {
    return h_action(h_in(charset, length), h__to_char, NULL);
  }
  HParser* not_in(const uint8_t *charset, size_t length) {
    return h_action(h_not_in(charset, length), h__to_char, NULL);
  }
 }

%extend HParser_ {
    SV* parse(const uint8_t* input, size_t length) {
      SV* hpt_to_perl(const struct HParsedToken_ *token);
      HParseResult *res = h_parse($self, input, length);
      if (res) {
	return hpt_to_perl(res->ast);
      } else {
	croak("Parse failure");
      }
    }
    bool compile(HParserBackend backend) {
        return h_compile($self, backend, NULL) == 0;
    }
}
