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

static bool ignore_isValidRegular(void *env) {
  HParser *p = (HParser*)env;
  return (p->vtable->isValidRegular(p->env));
}

static bool ignore_isValidCF(void *env) {
  HParser *p = (HParser*)env;
  return (p->vtable->isValidCF(p->env));
}

static HCFChoice* desugar_ignore(HAllocator *mm__, void *env) {
  HParser *p = (HParser*)env;
  return (p->vtable->desugar(mm__, p->env));
}

static const HParserVtable ignore_vt = {
  .parse = parse_ignore,
  .isValidRegular = ignore_isValidRegular,
  .isValidCF = ignore_isValidCF,
  .desugar = desugar_ignore,
};

const HParser* h_ignore(const HParser* p) {
  return h_ignore__m(&system_allocator, p);
}
const HParser* h_ignore__m(HAllocator* mm__, const HParser* p) {
  HParser* ret = h_new(HParser, 1);
  ret->vtable = &ignore_vt;
  ret->env = (void*)p;
  return ret;
}
