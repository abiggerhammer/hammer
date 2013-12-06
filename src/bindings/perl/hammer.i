%module hammer;
%begin %{
#include <unistd.h>
#include <stdbool.h>
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

%typemap(in) void*[] {
  if (!SvROK($input))
    SWIG_exception_fail(SWIG_TypeError, "Expected array ref");

  if (SvTYPE(SvRV($input)) != SVt_PVAV)
    SWIG_exception_fail(SWIG_TypeError, "Expected array ref");

  AV* av = (AV*) SvRV($input);
  size_t amax = av_len(av) + 1; // I want the length, not the top index...
  // TODO: is this array copied?
  $1 = malloc((amax+1) * sizeof(*$1));
  $1[amax] = NULL;
  for (int i = 0; i < amax; i++) {
    int res = SWIG_ConvertPtr(*av_fetch(av, i, 0), &($1[i]), SWIGTYPE_p_HParser_, 0|0);
    if (!SWIG_IsOK(res)) {
      SWIG_exception_fail(SWIG_ArgError(res), "Expected a list of parsers and only parsers");
    }
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

%define %combinator %rename("%(regex:/^h_(.*)$/\\1/)s") %enddef
  
%combinator h_end_p;
%combinator h_left;
%combinator h_middle;
%combinator h_right;
%combinator h_int_range;
%combinator h_whitespace;
%combinator h_nothing_p;

%combinator h_butnot;
%combinator h_difference;
%combinator h_xor;
%combinator h_many;
%combinator h_many1;
%combinator h_sepBy;
%combinator h_sepBy1;
%combinator h_repeat_n;
%combinator h_ignore;
%combinator h_optional;
%combinator h_epsilon_p;
%combinator h_and;
%combinator h_not;
%combinator h_indirect;
%combinator h_bind_indirect;

%include "../swig/hammer.i";
  

%{
  SV* hpt_to_perl(const HParsedToken *token) {
    // All values that this function returns have a refcount of exactly 1.
    SV *ret;
    if (token == NULL) {
      return newSV(0); // Same as TT_NONE
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
      AV* aret = newAV();
      av_extend(aret, token->token_data.seq->used);
      for (int i = 0; i < token->token_data.seq->used; i++) {
	av_store(aret, i, hpt_to_perl(token->token_data.seq->elements[i]));
      }
      return newRV_noinc((SV*)aret);
    }
    default:
      if (token->token_type == h_tt_perl) {
	return  SvREFCNT_inc((SV*)token->token_data.user);
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

 static HParsedToken* call_action(const HParseResult *p, void* user_data  ) {
   SV *func = (SV*)user_data;

   dSP;
   ENTER;
   SAVETMPS;
   PUSHMARK(SP);
   if (p->ast != NULL) {
     mXPUSHs(hpt_to_perl(p->ast));
   } else {
     mXPUSHs(newSV(0));
   }
   PUTBACK;
   
   int nret = call_sv(func, G_SCALAR);

   SPAGAIN;
   if (nret != 1)
     croak("Expected 1 return value, got %d", nret);
   
   HParsedToken *ret = h_arena_malloc(p->arena, sizeof(*ret));
   memset(ret, 0, sizeof(*ret));
   ret->token_type = h_tt_perl;
   ret->token_data.user = SvREFCNT_inc(POPs);
   if (p->ast != NULL) {
     ret->index = p->ast->index;
     ret->bit_offset = p->ast->bit_offset;
   }
   PUTBACK;
   FREETMPS;
   LEAVE;

   return ret;
 }

 static int call_predicate(HParseResult *p, void* user_data) {
   SV *func = (SV*)user_data;

   dSP;
   ENTER;
   SAVETMPS;
   PUSHMARK(SP);
   if (p->ast != NULL) {
     mXPUSHs(hpt_to_perl(p->ast));
   } else {
     mXPUSHs(newSV(0));
   }
   PUTBACK;
   
   int nret = call_sv(func, G_SCALAR);

   SPAGAIN;
   if (nret != 1)
     croak("Expected 1 return value, got %d", nret);

   SV* svret = POPs;
   int ret = SvTRUE(svret);
   PUTBACK;
   FREETMPS;
   LEAVE;

   return ret;
 }

%}
%inline {
  HParser* ch(uint8_t chr) {
    return h_action(h_ch(chr), h__to_char, NULL);
  }
  HParser* ch_range(uint8_t c0, uint8_t c1) {
    return h_action(h_ch_range(c0,c1), h__to_char, NULL);
  }
  HParser* h__in(const uint8_t *charset, size_t length) {
    return h_action(h_in(charset, length), h__to_char, NULL);
  }
  HParser* h__not_in(const uint8_t *charset, size_t length) {
    return h_action(h_not_in(charset, length), h__to_char, NULL);
  }
  HParser* action(HParser *parser, SV* sub) {
    return h_action(parser, call_action, SvREFCNT_inc(sub));
  }
  HParser* attr_bool(HParser *parser, SV* sub) {
    return h_attr_bool(parser, call_predicate, SvREFCNT_inc(sub));
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

%perlcode %{
  sub sequence {
    return hammerc::h_sequence__a([@_]);
  }
  sub choice {
    return hammerc::h_choice__a([@_]);
  }
  sub in {
    return h__in(join('',@_));
  }
  sub not_in {
    return h__not_in(join('',@_));
  }


  %}
