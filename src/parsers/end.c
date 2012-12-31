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

static const HParserVtable end_vt = {
  .parse = parse_end,
  .isValidRegular = h_true,
  .isValidCF = h_true,
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
