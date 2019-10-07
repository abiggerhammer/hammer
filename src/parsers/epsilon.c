#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif            
#include "parser_internal.h"

static HParseResult* parse_epsilon(void* env, HParseState* state) {
  (void)env;
  HParseResult* res = a_new(HParseResult, 1);
  res->ast = NULL;
  res->arena = state->arena;
  return res;
}

static bool epsilon_ctrvm(HRVMProg *prog, void* env) {
  return true;
}

#ifdef HAMMER_LLVM_BACKEND

static bool epsilon_llvm(HLLVMParserCompileContext *ctxt, void* env) {
  if (!ctxt) return false;

  LLVMBasicBlockRef epsilon_bb = LLVMAppendBasicBlock(ctxt->func, "epsilon");

  /* Basic block: epsilon */
  LLVMBuildBr(ctxt->builder, epsilon_bb);
  LLVMPositionBuilderAtEnd(ctxt->builder, epsilon_bb);

  /*
   * For epsilon we make a null-token parse result like with end, but we
   * do it unconditionally.
   */
  LLVMValueRef make_result_args[] = {
    ctxt->arena,
    LLVMConstNull(ctxt->llvm_parsedtokenptr)
  };
  LLVMValueRef result_ptr = LLVMBuildCall(ctxt->builder,
    LLVMGetNamedFunction(ctxt->mod, "make_result"),
    make_result_args, 2, "result_ptr");
  /* Return it */
  LLVMBuildRet(ctxt->builder, result_ptr);

  return true;
}

#endif

static const HParserVtable epsilon_vt = {
  .parse = parse_epsilon,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_epsilon,
  .compile_to_rvm = epsilon_ctrvm,
#ifdef HAMMER_LLVM_BACKEND
  .llvm = epsilon_llvm,
#endif
  .higher = false,
};

HParser* h_epsilon_p() {
  return h_epsilon_p__m(&system_allocator);
}
HParser* h_epsilon_p__m(HAllocator* mm__) {
  HParser *epsilon_p = h_new(HParser, 1);
  epsilon_p->desugared = NULL;
  epsilon_p->backend_data = NULL;
  epsilon_p->backend = 0;
  epsilon_p->vtable = &epsilon_vt;
  return epsilon_p;
}
