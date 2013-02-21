#include "parser_internal.h"

static HParseResult* parse_indirect(void* env, HParseState* state) {
  return h_do_parse(env, state);
}

static bool indirect_isValidCF(void *env) {
  HParser *p = (HParser*)env;
  HParser *inner = (HParser*)p->env;
  return inner->vtable->isValidCF(inner->env);
}

static HCFChoice* desugar_indirect(HAllocator *mm__, void *env) {
  HParser *p = (HParser*)env;
  HParser *inner = (HParser*)p->env;
  return inner->desugared;
}

static const HParserVtable indirect_vt = {
  .parse = parse_indirect,
  .isValidRegular = h_false,
  .isValidCF = indirect_isValidCF,
  .desugar = desugar_indirect,
};

void h_bind_indirect(HParser* indirect, const HParser* inner) {
  assert_message(indirect->vtable == &indirect_vt, "You can only bind an indirect parser");
  indirect->env = (void*)inner;
}

HParser* h_indirect() {
  return h_indirect__m(&system_allocator);
}
HParser* h_indirect__m(HAllocator* mm__) {
  HParser *res = h_new(HParser, 1);
  res->vtable = &indirect_vt;
  res->env = NULL;
  return res;
}
