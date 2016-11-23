#include <stdint.h>
#include <assert.h>
#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif
#include "parser_internal.h"

static HParseResult* parse_ch(void* env, HParseState *state) {
  uint8_t c = (uint8_t)(uintptr_t)(env);
  uint8_t r = (uint8_t)h_read_bits(&state->input_stream, 8, false);
  if (c == r) {
    HParsedToken *tok = a_new(HParsedToken, 1);    
    tok->token_type = TT_UINT; tok->uint = r;
    return make_result(state->arena, tok);
  } else {
    return NULL;
  }
}

static void desugar_ch(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_ADD_CHAR( (uint8_t)(uintptr_t)(env) );
}

static bool h_svm_action_ch(HArena *arena, HSVMContext *ctx, void* env) {
  // BUG: relies un undefined behaviour: int64_t is a signed uint64_t; not necessarily true on 32-bit
  HParsedToken *top = ctx->stack[ctx->stack_count-1];
  assert(top->token_type == TT_BYTES);
  uint64_t res = 0;
  for (size_t i = 0; i < top->bytes.len; i++)
    res = (res << 8) | top->bytes.token[i];   // TODO: Handle other endiannesses.
  top->uint = res; // possibly cast to signed through union
  top->token_type = TT_UINT;
  return true;
}

static bool ch_ctrvm(HRVMProg *prog, void* env) {
  uint8_t c = (uint8_t)(uintptr_t)(env);
  // TODO: Does this capture anything?
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  h_rvm_insert_insn(prog, RVM_MATCH, c | c << 8);
  h_rvm_insert_insn(prog, RVM_STEP, 0);
  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_ch, env));
  return true;
}

#ifdef HAMMER_LLVM_BACKEND

static bool ch_llvm(HLLVMParserCompileContext *ctxt, void* env) {
  // Build a new LLVM function to parse a character

  // Set up params for calls to h_read_bits() and h_arena_malloc()
  LLVMValueRef bits_args[3];
  bits_args[0] = ctxt->stream;
  bits_args[1] = LLVMConstInt(LLVMInt32Type(), 8, 0);
  bits_args[2] = LLVMConstInt(LLVMInt8Type(), 0, 0);

  // Set up basic blocks: entry, success and failure branches, then exit
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(ctxt->func, "ch_entry");
  LLVMBasicBlockRef success = LLVMAppendBasicBlock(ctxt->func, "ch_success");
  LLVMBasicBlockRef end = LLVMAppendBasicBlock(ctxt->func, "ch_end");

  // Basic block: entry
  LLVMBuildBr(ctxt->builder, entry);
  LLVMPositionBuilderAtEnd(ctxt->builder, entry);

  // Call to h_read_bits()
  // %read_bits = call i64 @h_read_bits(%struct.HInputStream_* %8, i32 8, i8 signext 0)
  LLVMValueRef bits = LLVMBuildCall(ctxt->builder,
      LLVMGetNamedFunction(ctxt->mod, "h_read_bits"), bits_args, 3, "read_bits");
  // %2 = trunc i64 %read_bits to i8
  LLVMValueRef r = LLVMBuildTrunc(ctxt->builder,
      bits, LLVMInt8Type(), ""); // do we actually need this?

  // Check if h_read_bits succeeded
  // %"c == r" = icmp eq i8 -94, %2 ; the -94 comes from c_
  uint8_t c_ = (uint8_t)(uintptr_t)(env);
  LLVMValueRef c = LLVMConstInt(LLVMInt8Type(), c_, 0);
  LLVMValueRef icmp = LLVMBuildICmp(ctxt->builder, LLVMIntEQ, c, r, "c == r");

  // Branch so success or failure basic block, as appropriate
  // br i1 %"c == r", label %ch_success, label %ch_fail
  LLVMBuildCondBr(ctxt->builder, icmp, success, end);

  // Basic block: success
  LLVMPositionBuilderAtEnd(ctxt->builder, success);

  /* Make a token */
  LLVMValueRef mr;
  h_llvm_make_tt_suint(ctxt, 8, 0, r, &mr);

  // br label %ch_end
  LLVMBuildBr(ctxt->builder, end);
  
  // Basic block: end
  LLVMPositionBuilderAtEnd(ctxt->builder, end);
  // %rv = phi %struct.HParseResult_.3* [ %make_result, %ch_success ], [ null, %ch_entry ]
  LLVMValueRef rv = LLVMBuildPhi(ctxt->builder, ctxt->llvm_parseresultptr, "rv");
  LLVMBasicBlockRef rv_phi_incoming_blocks[] = {
    success,
    entry
    };
  LLVMValueRef rv_phi_incoming_values[] = {
    mr,
    LLVMConstNull(ctxt->llvm_parseresultptr)
    };
  LLVMAddIncoming(rv, rv_phi_incoming_values, rv_phi_incoming_blocks, 2);
  // ret %struct.HParseResult_.3* %rv
  LLVMBuildRet(ctxt->builder, rv);
  return true;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */

static const HParserVtable ch_vt = {
  .parse = parse_ch,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_ch,
  .compile_to_rvm = ch_ctrvm,
#ifdef HAMMER_LLVM_BACKEND
  .llvm = ch_llvm,
#endif
  .higher = false,
};

HParser* h_ch(const uint8_t c) {
  return h_ch__m(&system_allocator, c);
}
HParser* h_ch__m(HAllocator* mm__, const uint8_t c) {  
  return h_new_parser(mm__, &ch_vt, (void *)(uintptr_t)c);
}
