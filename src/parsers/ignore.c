#include "parser_internal.h"

static HParseResult* parse_ignore(void* env, HParseState* state) {
  HParseResult *res0 = h_do_parse((HParser*)env, state);
  if (!res0)
    return NULL;
  HParseResult *res = a_new(HParseResult, 1);
  res->ast = NULL;
  res->arena = state->arena;
  return res;
}

static const HParserVtable ignore_vt = {
  .parse = parse_ignore,
};

const HParser* h_ignore(const HParser* p) {
  HParser* ret = g_new(HParser, 1);
  ret->vtable = &ignore_vt;
  ret->env = (void*)p;
  return ret;
}
