#include "parser_internal.h"

static HParseResult* parse_epsilon(void* env, HParseState* state) {
  (void)env;
  HParseResult* res = a_new(HParseResult, 1);
  res->ast = NULL;
  res->arena = state->arena;
  return res;
}

static const HParserVtable epsilon_vt = {
  .parse = parse_epsilon,
};

const HParser* h_epsilon_p() {
  HParser *res = g_new(HParser, 1);
  res->vtable = &epsilon_vt;
  res->env = NULL;
  return res;
}
