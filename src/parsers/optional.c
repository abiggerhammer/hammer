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

static bool opt_isValidRegular(void *env) {
  HParser *p = (HParser*) env;
  return p->vtable->isValidRegular(p->env);
}

static bool opt_isValidCF(void *env) {
  HParser *p = (HParser*) env;
  return p->vtable->isValidCF(p->env);
}

static bool opt_ctrvm(HRVMProg *prog, void* env) {
  uint16_t insn = h_rvm_insert_insn(prog, RVM_FORK, 0);
  HParser *p = (HParser*) env;
  if (!h_compile_regex(prog, p->env))
    return false;
  h_rvm_patch_arg(prog, insn, h_rvm_get_ip(prog));
  return true;
}

static const HParserVtable optional_vt = {
  .parse = parse_optional,
  .isValidRegular = opt_isValidRegular,
  .isValidCF = opt_isValidCF,
  .compile_to_rvm = opt_ctrvm,
};

const HParser* h_optional(const HParser* p) {
  return h_optional__m(&system_allocator, p);
}
const HParser* h_optional__m(HAllocator* mm__, const HParser* p) {
  // TODO: re-add this
  //assert_message(p->vtable != &ignore_vt, "Thou shalt ignore an option, rather than the other way 'round.");
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &optional_vt;
  ret->env = (void*)p;
  return ret;
}

