#include "parser_internal.h"

static HParseResult* parse_not(void* env, HParseState* state) {
  HInputStream bak = state->input_stream;
  if (h_do_parse((HParser*)env, state))
    return NULL;
  else {
    state->input_stream = bak;
    return make_result(state, NULL);
  }
}

static HCFChoice* desugar_not(HAllocator *mm__, void *env) {
  assert_message(0, "'h_not' is not context-free, can't be desugared");
  return NULL;
}

static const HParserVtable not_vt = {
  .parse = parse_not,
  .isValidRegular = h_false,  /* see and.c for why */
  .isValidCF = h_false,       /* also see and.c for why */
  .desugar = desugar_not,
};

const HParser* h_not(const HParser* p) {
  return h_not__m(&system_allocator, p);
}
const HParser* h_not__m(HAllocator* mm__, const HParser* p) {
  HParser *res = h_new(HParser, 1);
  res->vtable = &not_vt;
  res->env = (void*)p;
  return res;
}
