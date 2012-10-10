#include "parser_internal.h"

static HParseResult *parse_and(void* env, HParseState* state) {
  HInputStream bak = state->input_stream;
  HParseResult *res = h_do_parse((HParser*)env, state);
  state->input_stream = bak;
  if (res)
    return make_result(state, NULL);
  return NULL;
}

static const HParserVtable and_vt = {
  .parse = parse_and,
};


const HParser* h_and(const HParser* p) {
  return h_and__m(&system_allocator, p);
}
const HParser* h_and__m(HAllocator* mm__, const HParser* p) {
  // zero-width postive lookahead
  HParser *res = h_new(HParser, 1);
  res->env = (void*)p;
  res->vtable = &and_vt;
  return res;
}
