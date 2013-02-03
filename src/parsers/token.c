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

static HCFChoice* desugar_token(HAllocator *mm__, void *env) {
  HToken *tok = (HToken*)env;
  HCFSequence *seq = h_new(HCFSequence, 1);
  seq->items = h_new(HCFChoice*, 1+tok->len);
  for (size_t i=0; i<tok->len; ++i) {
    seq->items[i] = h_new(HCFChoice, 1);
    seq->items[i]->type = HCF_CHAR;
    seq->items[i]->chr = tok->str[i];
  }
  seq->items[tok->len] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = seq;
  ret->seq[1] = NULL;
  ret->action = NULL;
  return ret;
}

const HParserVtable token_vt = {
  .parse = parse_token,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_token,
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
