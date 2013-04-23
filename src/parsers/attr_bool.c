#include "parser_internal.h"

typedef struct {
  const HParser *p;
  HPredicate pred;
} HAttrBool;

static HParseResult* parse_attr_bool(void *env, HParseState *state) {
  HAttrBool *a = (HAttrBool*)env;
  HParseResult *res = h_do_parse(a->p, state);
  if (res && res->ast) {
    if (a->pred(res))
      return res;
    else
      return NULL;
  } else
    return NULL;
}

static bool ab_isValidRegular(void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return ab->p->vtable->isValidRegular(ab->p->env);
}

static bool ab_isValidCF(void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return ab->p->vtable->isValidCF(ab->p->env);
}

static bool ab_ctrvm(HRVMProg *prog, void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return h_compile_regex(prog, ab->p);
}

static const HParserVtable attr_bool_vt = {
  .parse = parse_attr_bool,
  .isValidRegular = ab_isValidRegular,
  .isValidCF = ab_isValidCF,
  .compile_to_rvm = ab_ctrvm,
};


const HParser* h_attr_bool(const HParser* p, HPredicate pred) {
  return h_attr_bool__m(&system_allocator, p, pred);
}
const HParser* h_attr_bool__m(HAllocator* mm__, const HParser* p, HPredicate pred) { 
  HParser *res = h_new(HParser, 1);
  res->vtable = &attr_bool_vt;
  HAttrBool *env = h_new(HAttrBool, 1);
  env->p = p;
  env->pred = pred;
  res->env = (void*)env;
  return res;
}
