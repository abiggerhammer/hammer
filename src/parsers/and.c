#include "parser_internal.h"

static HParseResult *parse_and(void* env, HParseState* state) {
  HInputStream bak = state->input_stream;
  HParseResult *res = h_do_parse((HParser*)env, state);
  state->input_stream = bak;
  if (res)
    return make_result(state->arena, NULL);
  return NULL;
}

static const HParserVtable and_vt = {
  .parse = parse_and,
  .isValidRegular = h_false, /* TODO: strictly speaking this should be regular,
				but it will be a huge amount of work and difficult
				to get right, so we're leaving it for a future
				revision. --mlp, 18/12/12 */
  .isValidCF = h_false,      /* despite TODO above, this remains false. */
  .compile_to_rvm = h_not_regular,
  .higher = true,
};


HParser* h_and(const HParser* p) {
  return h_and__m(&system_allocator, p);
}
HParser* h_and__m(HAllocator* mm__, const HParser* p) {
  // zero-width postive lookahead
  void* env = (void*)p;
  return h_new_parser(mm__, &and_vt, env);
}
