#include "parser_internal.h"

static HParseResult* parse_nothing() {
  // not a mistake, this parser always fails
  return NULL;
}

static bool nothing_ctrvm(HRVMProg *prog, void* env) {
  h_rvm_insert_insn(prog, RVM_MATCH, 0x00FF);
  return true;
}

static const HParserVtable nothing_vt = {
  .parse = parse_nothing,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .compile_to_rvm = nothing_ctrvm,
};

const HParser* h_nothing_p() {
  return h_nothing_p__m(&system_allocator);
}
const HParser* h_nothing_p__m(HAllocator* mm__) { 
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &nothing_vt; ret->env = NULL;
  return (const HParser*)ret;
}
