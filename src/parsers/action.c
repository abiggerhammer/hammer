#include "parser_internal.h"

typedef struct {
  const HParser *p;
  HAction action;
} HParseAction;

static HParseResult* parse_action(void *env, HParseState *state) {
  HParseAction *a = (HParseAction*)env;
  if (a->p && a->action) {
    HParseResult *tmp = h_do_parse(a->p, state);
    //HParsedToken *tok = a->action(h_do_parse(a->p, state));
    if(tmp) {
        const HParsedToken *tok = a->action(tmp);
        return make_result(state, (HParsedToken*)tok);
    } else
      return NULL;
  } else // either the parser's missing or the action's missing
    return NULL;
}

static HCFChoice* desugar_action(HAllocator *mm__, void *env) {
  HParseAction *a = (HParseAction*)env;
  HCFSequence *seq = h_new(HCFSequence, 1);
  seq->items = h_new(HCFChoice*, 2);
  seq->items[0] = h_desugar(mm__, a->p);
  seq->items[1] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = seq;
  ret->seq[1] = NULL;
  ret->action = a->action;
  return ret;
}

static bool action_isValidRegular(void *env) {
  HParseAction *a = (HParseAction*)env;
  return a->p->vtable->isValidRegular(a->p->env);
}

static bool action_isValidCF(void *env) {
  HParseAction *a = (HParseAction*)env;
  return a->p->vtable->isValidCF(a->p->env);
}

static const HParserVtable action_vt = {
  .parse = parse_action,
  .isValidRegular = action_isValidRegular,
  .isValidCF = action_isValidCF,
  .desugar = desugar_action,
};

const HParser* h_action(const HParser* p, const HAction a) {
  return h_action__m(&system_allocator, p, a);
}

const HParser* h_action__m(HAllocator* mm__, const HParser* p, const HAction a) {
  HParser *res = h_new(HParser, 1);
  res->vtable = &action_vt;
  HParseAction *env = h_new(HParseAction, 1);
  env->p = p;
  env->action = a;
  res->env = (void*)env;
  return res;
}
