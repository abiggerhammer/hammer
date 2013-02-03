#include "parser_internal.h"

static HParseResult* parse_ch(void* env, HParseState *state) {
  uint8_t c = (uint8_t)(unsigned long)(env);
  uint8_t r = (uint8_t)h_read_bits(&state->input_stream, 8, false);
  if (c == r) {
    HParsedToken *tok = a_new(HParsedToken, 1);    
    tok->token_type = TT_UINT; tok->uint = r;
    return make_result(state, tok);
  } else {
    return NULL;
  }
}

static HCFChoice* desugar_ch(HAllocator *mm__, void *env) {
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHAR;
  ret->chr = (uint8_t)(unsigned long)(env);
  ret->action = NULL;
  return ret;
}

static const HParserVtable ch_vt = {
  .parse = parse_ch,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_ch,
};

const HParser* h_ch(const uint8_t c) {
  return h_ch__m(&system_allocator, c);
}
const HParser* h_ch__m(HAllocator* mm__, const uint8_t c) {  
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &ch_vt;
  ret->env = (void*)(unsigned long)(c);
  return (const HParser*)ret;
}
