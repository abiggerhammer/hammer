#include "parser_internal.h"

static HParseResult* parse_optional(void* env, HParseState* state) {
  HInputStream bak = state->input_stream;
  HParseResult *res0 = h_do_parse((HParser*)env, state);
  if (res0)
    return res0;
  state->input_stream = bak;
  HParsedToken *ast = a_new(HParsedToken, 1);
  ast->token_type = TT_NONE;
  return make_result(state, ast);
}

static const HParserVtable optional_vt = {
  .parse = parse_optional,
};

const HParser* h_optional(const HParser* p) {
  // TODO: re-add this
  //assert_message(p->vtable != &ignore_vt, "Thou shalt ignore an option, rather than the other way 'round.");
  HParser *ret = g_new(HParser, 1);
  ret->vtable = &optional_vt;
  ret->env = (void*)p;
  return ret;
}

