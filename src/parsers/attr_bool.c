#include <assert.h>
#include "parser_internal.h"

typedef struct {
  const HParser *p;
  HPredicate pred;
  void* user_data;
} HAttrBool;

static HParseResult* parse_attr_bool(void *env, HParseState *state) {
  HAttrBool *a = (HAttrBool*)env;
  HParseResult *res = h_do_parse(a->p, state);
  if (res && res->ast) {
    if (a->pred(res, a->user_data))
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

/* Strictly speaking, this does not adhere to the definition of context-free.
   However, for the purpose of making it easier to handle weird constraints on
   fields, we've decided to allow boolean attribute validation at parse time
   for now. We might change our minds later. */

static bool ab_isValidCF(void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return ab->p->vtable->isValidCF(ab->p->env);
}

static void desugar_ab(HAllocator *mm__, HCFStack *stk__, void *env) {
      
  HAttrBool *a = (HAttrBool*)env;
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      HCFS_DESUGAR(a->p);
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->pred = a->pred;
    HCFS_THIS_CHOICE->reshape = h_act_first;
    HCFS_THIS_CHOICE->user_data = a->user_data;
  } HCFS_END_CHOICE();
}

static bool h_svm_action_attr_bool(HArena *arena, HSVMContext *ctx, void* arg) {
  HParseResult res;
  HAttrBool *ab = arg;
  assert(ctx->stack_count >= 1);
  if (ctx->stack[ctx->stack_count-1]->token_type != TT_MARK) {
    assert(ctx->stack_count >= 2 && ctx->stack[ctx->stack_count-2]->token_type == TT_MARK);
    ctx->stack_count--;
    res.ast = ctx->stack[ctx->stack_count-1] = ctx->stack[ctx->stack_count];
    // mark replaced.
  } else {
    ctx->stack_count--;
    res.ast = NULL;
  }
  res.arena = arena;
  return ab->pred(&res, ab->user_data);
}

static bool ab_ctrvm(HRVMProg *prog, void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  if (!h_compile_regex(prog, ab->p))
    return false;
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_attr_bool, ab));
  return true;
}

static const HParserVtable attr_bool_vt = {
  .parse = parse_attr_bool,
  .isValidRegular = ab_isValidRegular,
  .isValidCF = ab_isValidCF,
  .desugar = desugar_ab,
  .compile_to_rvm = ab_ctrvm,
  .higher = true,
};


HParser* h_attr_bool(const HParser* p, HPredicate pred, void* user_data) {
  return h_attr_bool__m(&system_allocator, p, pred, user_data);
}
HParser* h_attr_bool__m(HAllocator* mm__, const HParser* p, HPredicate pred, void* user_data) { 
  HAttrBool *env = h_new(HAttrBool, 1);
  env->p = p;
  env->pred = pred;
  env->user_data = user_data;
  return h_new_parser(mm__, &attr_bool_vt, env);
}
