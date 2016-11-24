#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif
#include "parser_internal.h"

static HParseResult* parse_nothing() {
  // not a mistake, this parser always fails
  return NULL;
}

static void desugar_nothing(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_BEGIN_CHOICE() {
  } HCFS_END_CHOICE();
}

static bool nothing_ctrvm(HRVMProg *prog, void* env) {
  h_rvm_insert_insn(prog, RVM_MATCH, 0x0000);
  h_rvm_insert_insn(prog, RVM_MATCH, 0xFFFF);
  return true;
}

#ifdef HAMMER_LLVM_BACKEND

static bool nothing_llvm(HLLVMParserCompileContext *ctxt, void* env) {
  if (!ctxt) return false;

  /* This one just always returns NULL */
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(ctxt->func, "nothing_entry");
  LLVMBuildBr(ctxt->builder, entry);
  LLVMPositionBuilderAtEnd(ctxt->builder, entry);

  LLVMBuildRet(ctxt->builder, LLVMConstNull(ctxt->llvm_parseresultptr));

  return true;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */

static const HParserVtable nothing_vt = {
  .parse = parse_nothing,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_nothing,
  .compile_to_rvm = nothing_ctrvm,
#ifdef HAMMER_LLVM_BACKEND
  .llvm = nothing_llvm,
#endif
  .higher = false,
};

HParser* h_nothing_p() {
  return h_nothing_p__m(&system_allocator);
}
HParser* h_nothing_p__m(HAllocator* mm__) { 
  return h_new_parser(mm__, &nothing_vt, NULL);
}
