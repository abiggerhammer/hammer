#include <stdint.h>
#include <assert.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "parser_internal.h"
#include "../llvm.h"

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

static bool ch_llvm(LLVMBuilderRef builder, LLVMValueRef func, LLVMModuleRef mod, void* env) {
  // Build a new LLVM function to parse a character

  // Set up params for calls to h_read_bits() and h_arena_malloc()
  LLVMValueRef bits_args[3];
  LLVMValueRef stream = LLVMGetFirstParam(func);
  stream = LLVMBuildBitCast(builder, stream, llvm_inputstreamptr, "stream");
  bits_args[0] = stream;
  bits_args[1] = LLVMConstInt(LLVMInt32Type(), 8, 0);
  bits_args[2] = LLVMConstInt(LLVMInt8Type(), 0, 0);
  LLVMValueRef arena = LLVMGetLastParam(func);

  // Set up basic blocks: entry, success and failure branches, then exit
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "ch_entry");
  LLVMBasicBlockRef success = LLVMAppendBasicBlock(func, "ch_success");
  LLVMBasicBlockRef fail = LLVMAppendBasicBlock(func, "ch_fail");
  LLVMBasicBlockRef end = LLVMAppendBasicBlock(func, "ch_end");

  // Basic block: entry
  LLVMPositionBuilderAtEnd(builder, entry);
  // Allocate stack space for our return value
  // %ret = alloca %struct.HParseResult_*, align 8
  LLVMValueRef ret = LLVMBuildAlloca(builder, llvm_parseresultptr, "ret");

  // Call to h_read_bits()
  // %read_bits = call i64 @h_read_bits(%struct.HInputStream_* %8, i32 8, i8 signext 0)
  LLVMValueRef bits = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "h_read_bits"), bits_args, 3, "read_bits");
  // %2 = trunc i64 %read_bits to i8
  LLVMValueRef r = LLVMBuildTrunc(builder, bits, LLVMInt8Type(), ""); // do we actually need this?

  // Check if h_read_bits succeeded
  // %"c == r" = icmp eq i8 -94, %2 ; the -94 comes from c_
  uint8_t c_ = (uint8_t)(uintptr_t)(env);
  LLVMValueRef c = LLVMConstInt(LLVMInt8Type(), c_, 0);
  LLVMValueRef icmp = LLVMBuildICmp(builder, LLVMIntEQ, c, r, "c == r");

  // Branch so success or failure basic block, as appropriate
  // br i1 %"c == r", label %ch_success, label %ch_fail
  LLVMBuildCondBr(builder, icmp, success, fail);

  // Basic block: success
  LLVMPositionBuilderAtEnd(builder, success);

  // Set up call to h_arena_malloc() for a new HParsedToken
  LLVMValueRef tok_size = LLVMConstInt(LLVMInt32Type(), sizeof(HParsedToken), 0);
  LLVMValueRef amalloc_args[] = { arena, tok_size };
  // %h_arena_malloc = call void* @h_arena_malloc(%struct.HArena_.1* %1, i32 48)
  LLVMValueRef amalloc = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "h_arena_malloc"), amalloc_args, 2, "h_arena_malloc");
  // %3 = bitcast void* %h_arena_malloc to %struct.HParsedToken_.2*
  LLVMValueRef tok = LLVMBuildBitCast(builder, amalloc, llvm_parsedtokenptr, "");

  // tok->token_type = TT_UINT;
  //
  // %token_type = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 0
  LLVMValueRef toktype = LLVMBuildStructGEP(builder, tok, 0, "token_type");
  // store i32 8, i32* %token_type
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 8, 0), toktype);

  // tok->uint = r;
  //
  // %token_data = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 1
  LLVMValueRef tokdata = LLVMBuildStructGEP(builder, tok, 1, "token_data");
  // %r = zext i8 %2 to i64
  // store i64 %r, i64* %token_data
  LLVMBuildStore(builder, LLVMBuildZExt(builder, r, LLVMInt64Type(), "r"), tokdata);
  // %t_index = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 2
  LLVMValueRef tokindex = LLVMBuildStructGEP(builder, tok, 2, "t_index");
  // %s_index = getelementptr inbounds %struct.HInputStream_.0, %struct.HInputStream_.0* %0, i32 0, i32 2
  LLVMValueRef streamindex = LLVMBuildStructGEP(builder, stream, 2, "s_index");
  // %4 = load i64, i64* %s_index
  // store i64 %4, i64* %t_index
  LLVMBuildStore(builder, LLVMBuildLoad(builder, streamindex, ""), tokindex);
  LLVMValueRef tokbitlen = LLVMBuildStructGEP(builder, tok, 3, "bit_length");
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt64Type(), 8, 0), tokbitlen);

  // Now call make_result()
  // %make_result = call %struct.HParseResult_.3* @make_result(%struct.HArena_.1* %1, %struct.HParsedToken_.2* %3)
  LLVMValueRef result_args[] = { arena, tok };
  LLVMValueRef mr = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "make_result"), result_args, 2, "make_result");

  // Store for return
  // store %struct.HParseResult_.3* %make_result, %struct.HParseResult_.3** %ret
  LLVMBuildStore(builder, mr, ret);
  // br label %ch_end
  LLVMBuildBr(builder, end);
  
  // Basic block: failure
  LLVMPositionBuilderAtEnd(builder, fail);
  // store %struct.HParseResult_.3* null, %struct.HParseResult_.3** %ret
  LLVMBuildStore(builder, LLVMConstNull(llvm_parseresultptr), ret);
  // br label %ch_end
  LLVMBuildBr(builder, end);

  // Basic block: end
  LLVMPositionBuilderAtEnd(builder, end);
  // %rv = load %struct.HParseResult_.3*, %struct.HParseResult_.3** %ret
  LLVMValueRef rv = LLVMBuildLoad(builder, ret, "rv");
  // ret %struct.HParseResult_.3* %rv
  LLVMBuildRet(builder, rv);
  return true;
}

static const HParserVtable ch_vt = {
  .parse = parse_ch,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_ch,
  .compile_to_rvm = ch_ctrvm,
  .llvm = ch_llvm,
  .higher = false,
};

HParser* h_ch(const uint8_t c) {
  return h_ch__m(&system_allocator, c);
}
HParser* h_ch__m(HAllocator* mm__, const uint8_t c) {  
  return h_new_parser(mm__, &ch_vt, (void *)(uintptr_t)c);
}
