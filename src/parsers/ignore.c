#include <assert.h>
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

static void desugar_ignore(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      HCFS_DESUGAR( (HParser*)env );
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = h_act_ignore;
  } HCFS_END_CHOICE();
}

static bool h_svm_action_pop(HArena *arena, HSVMContext *ctx, void* arg) {
  assert(ctx->stack_count > 0);
  ctx->stack_count--;
  return true;
}

static bool ignore_ctrvm(HRVMProg *prog, void *env) {
  HParser *p = (HParser*)env;
  h_compile_regex(prog, p);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_pop, NULL));
  return true;
}

static const HParserVtable ignore_vt = {
  .parse = parse_ignore,
  .isValidRegular = ignore_isValidRegular,
  .isValidCF = ignore_isValidCF,
  .desugar = desugar_ignore,
  .compile_to_rvm = ignore_ctrvm,
  .higher = true,
};

HParser* h_ignore(const HParser* p) {
  return h_ignore__m(&system_allocator, p);
}
HParser* h_ignore__m(HAllocator* mm__, const HParser* p) {
  void* env = (void*)p;
  return h_new_parser(mm__, &ignore_vt, env);
}
