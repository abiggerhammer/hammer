#include "parser_internal.h"


typedef struct {
  const HParser *p;
  int64_t lower;
  int64_t upper;
} HRange;

static HParseResult* parse_int_range(void *env, HParseState *state) {
  HRange *r_env = (HRange*)env;
  HParseResult *ret = h_do_parse(r_env->p, state);
  if (!ret || !ret->ast)
    return NULL;
  switch(ret->ast->token_type) {
  case TT_SINT:
    if (r_env->lower <= ret->ast->sint && r_env->upper >= ret->ast->sint)
      return ret;
    else
      return NULL;
  case TT_UINT:
    if ((uint64_t)r_env->lower <= ret->ast->uint && (uint64_t)r_env->upper >= ret->ast->uint)
      return ret;
    else
      return NULL;
  default:
    return NULL;
  }
}

static const HParserVtable int_range_vt = {
  .parse = parse_int_range,
};

const HParser* h_int_range(const HParser *p, const int64_t lower, const int64_t upper) {
  return h_int_range__m(&system_allocator, p, lower, upper);
}
const HParser* h_int_range__m(HAllocator* mm__, const HParser *p, const int64_t lower, const int64_t upper) {
  // p must be an integer parser, which means it's using parse_bits
  // TODO: re-add this check
  //assert_message(p->vtable == &bits_vt, "int_range requires an integer parser"); 

  // and regardless, the bounds need to fit in the parser in question
  // TODO: check this as well.

  HRange *r_env = h_new(HRange, 1);
  r_env->p = p;
  r_env->lower = lower;
  r_env->upper = upper;
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &int_range_vt;
  ret->env = (void*)r_env;
  return ret;
}
