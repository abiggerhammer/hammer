#include "parser_internal.h"

typedef struct {
  uint8_t *str;
  uint8_t len;
} HToken;



static HParseResult* parse_token(void *env, HParseState *state) {
  HToken *t = (HToken*)env;
  for (int i=0; i<t->len; ++i) {
    uint8_t chr = (uint8_t)h_read_bits(&state->input_stream, 8, false);
    if (t->str[i] != chr) {
      return NULL;
    }
  }
  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_BYTES; tok->bytes.token = t->str; tok->bytes.len = t->len;
  return make_result(state, tok);
}

static bool token_ctrvm(HRVMProg *prog, void *env) {
  HToken *t = (HToken*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (int i=0; i<t->len; ++i) {
    h_rvm_insert_insn(prog, RVM_MATCH, t->str[i] & t->str[i] << 8);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
  }
  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  return true;
}

const HParserVtable token_vt = {
  .parse = parse_token,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .compile_to_rvm = token_ctrvm,
};

const HParser* h_token(const uint8_t *str, const size_t len) {
  return h_token__m(&system_allocator, str, len);
}
const HParser* h_token__m(HAllocator* mm__, const uint8_t *str, const size_t len) { 
  HToken *t = h_new(HToken, 1);
  t->str = (uint8_t*)str, t->len = len;
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &token_vt;
  ret->env = t;
  return (const HParser*)ret;
}
