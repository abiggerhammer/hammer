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
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_epsilon,
};

static const HParser epsilon_p = {
  .vtable = &epsilon_vt,
  .env = NULL
};

const HParser* h_epsilon_p() {
  return &epsilon_p;
}
const HParser* h_epsilon_p__m(HAllocator* mm__) {
  return &epsilon_p;
}
