#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif
#include "parser_internal.h"

static HParseResult* parse_end(void *env, HParseState *state) {
  if (state->input_stream.index == state->input_stream.length) {
    HParseResult *ret = a_new(HParseResult, 1);
    ret->ast = NULL;
    return ret;
  } else {
    return NULL;
  }
}

static void desugar_end(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_ADD_END();
}

static bool end_ctrvm(HRVMProg *prog, void *env) {
  h_rvm_insert_insn(prog, RVM_EOF, 0);
  return true;
}

#ifdef HAMMER_LLVM_BACKEND

static bool end_llvm(HLLVMParserCompileContext *ctxt, void* env) {
  if (!ctxt) return false;

  /* Set up some basic blocks */
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(ctxt->func, "end_entry");
  LLVMBasicBlockRef success = LLVMAppendBasicBlock(ctxt->func, "end_success");
  LLVMBasicBlockRef end = LLVMAppendBasicBlock(ctxt->func, "end_end");

  /* Basic block: entry */
  LLVMBuildBr(ctxt->builder, entry);
  LLVMPositionBuilderAtEnd(ctxt->builder, entry);

  /*
   * This needs to test if we're at the end of the input stream by
   * comparing the index and length fields; build a struct GEP to
   * get at their values.
   */
  LLVMValueRef gep_indices[2];
  /* The struct itself */
  gep_indices[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
  /* The index field (see HInputStream in internal.h */
  gep_indices[1] = LLVMConstInt(LLVMInt32Type(), 2, 0);
  /* GEP */
  LLVMValueRef index_ptr = LLVMBuildGEP(ctxt->builder,
      ctxt->stream, gep_indices, 2, "index_ptr");
  /* The length field (see HInputStream in internal.h */
  gep_indices[1] = LLVMConstInt(LLVMInt32Type(), 3, 0);
  /* GEP */
  LLVMValueRef length_ptr = LLVMBuildGEP(ctxt->builder,
      ctxt->stream, gep_indices, 2, "length_ptr");
  /* Now load them */
  LLVMValueRef index = LLVMBuildLoad(ctxt->builder,
      index_ptr, "index");
  LLVMValueRef length = LLVMBuildLoad(ctxt->builder,
      length_ptr, "length");
  /* Compare */
  LLVMValueRef icmp = LLVMBuildICmp(ctxt->builder, LLVMIntEQ, index, length, "index == length");
  /* Branch on comparison */
  LLVMBuildCondBr(ctxt->builder, icmp, success, end);

  /* Basic block: success */
  LLVMPositionBuilderAtEnd(ctxt->builder, success);
  /* Set up a call to h_arena_malloc() to get an HParseResult */
  LLVMValueRef make_result_args[] = {
    ctxt->arena,
    LLVMConstNull(ctxt->llvm_parsedtokenptr)
  };
  LLVMValueRef result_ptr = LLVMBuildCall(ctxt->builder,
      LLVMGetNamedFunction(ctxt->mod, "make_result"),
      make_result_args, 2, "result_ptr");

  /* Branch to end */
  LLVMBuildBr(ctxt->builder, end);

  /* Basic block: end */
  LLVMPositionBuilderAtEnd(ctxt->builder, end);
  /* Set up a phi depending on whether we have a token or not */
  LLVMValueRef rv = LLVMBuildPhi(ctxt->builder, ctxt->llvm_parseresultptr, "rv");
  LLVMBasicBlockRef rv_phi_incoming_blocks[] = {
    success,
    entry
  };
  LLVMValueRef rv_phi_incoming_values[] = {
    result_ptr,
    LLVMConstNull(ctxt->llvm_parseresultptr)
  };
  LLVMAddIncoming(rv, rv_phi_incoming_values, rv_phi_incoming_blocks, 2);
  /* Return it */
  LLVMBuildRet(ctxt->builder, rv);

  return true;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */

static const HParserVtable end_vt = {
  .parse = parse_end,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_end,
  .compile_to_rvm = end_ctrvm,
#ifdef HAMMER_LLVM_BACKEND
  .llvm = end_llvm,
#endif
  .higher = false,
};

HParser* h_end_p() {
  return h_end_p__m(&system_allocator);
}

HParser* h_end_p__m(HAllocator* mm__) { 
  return h_new_parser(mm__, &end_vt, NULL);
}
