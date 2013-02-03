#include "parser_internal.h"

static HParseResult* parse_end(void *env, HParseState *state) {
  if (state->input_stream.index == state->input_stream.length) {
    HParseResult *ret = a_new(HParseResult, 1);
    ret->ast = NULL;
    return ret;
  } else {
    return NULL;
  }
}

static HCFChoice* desugar_end(HAllocator *mm__, void *env) {
  static HCFChoice ret = {
    .type = HCF_END
  };
  return &ret;
}

static const HParserVtable end_vt = {
  .parse = parse_end,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_end,
};

const HParser* h_end_p() {
  return h_end_p__m(&system_allocator);
}

const HParser* h_end_p__m(HAllocator* mm__) { 
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &end_vt;
  ret->env = NULL;
  return (const HParser*)ret;
}
