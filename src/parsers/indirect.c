#include "parser_internal.h"

typedef struct HIndirectEnv_ {
  const HParser* parser;
  bool touched;
} HIndirectEnv;

static HParseResult* parse_indirect(void* env, HParseState* state) {
  return h_do_parse(((HIndirectEnv*)env)->parser, state);
}

static bool indirect_isValidCF(void *env) {
  HIndirectEnv *ie = (HIndirectEnv*)env;
  if (ie->touched)
    return true;
  ie->touched = true;
  const HParser *p = ie->parser;
  // self->vtable->isValidCF = h_true;
  bool ret = p->vtable->isValidCF(p->env);
  ie->touched = false;
  return ret;
}

static void desugar_indirect(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_DESUGAR( ((HIndirectEnv *)env)->parser );
}

static const HParserVtable indirect_vt = {
  .parse = parse_indirect,
  .isValidRegular = h_false,
  .isValidCF = indirect_isValidCF,
  .desugar = desugar_indirect,
  .compile_to_rvm = h_not_regular,
  .higher = true,
};

void h_bind_indirect__m(HAllocator *mm__, HParser* indirect, const HParser* inner) {
  h_bind_indirect(indirect, inner);
}

void h_bind_indirect(HParser* indirect, const HParser* inner) {
  assert_message(indirect->vtable == &indirect_vt, "You can only bind an indirect parser");
  ((HIndirectEnv*)indirect->env)->parser = inner;
}

HParser* h_indirect() {
  return h_indirect__m(&system_allocator);
}
HParser* h_indirect__m(HAllocator* mm__) {
  HIndirectEnv *env = h_new(HIndirectEnv, 1);
  env->parser = NULL;
  env->touched = false;
  return h_new_parser(mm__, &indirect_vt, env);
}
