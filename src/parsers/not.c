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

static const HParserVtable not_vt = {
  .parse = parse_not,
};

const HParser* h_not(const HParser* p) {
  HParser *res = g_new(HParser, 1);
  res->vtable = &not_vt;
  res->env = (void*)p;
  return res;
}
