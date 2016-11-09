#include <stdint.h>
#include <assert.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
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

static bool ch_llvm(LLVMBuilderRef builder, LLVMModuleRef mod, void* env) {
  uint8_t c_ = (uint8_t)(uintptr_t)(env);
  LLVMValueRef c = LLVMConstInt(LLVMInt8Type(), c_, 0);
  LLVMTypeRef param_types[] = {
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HInputStream_"), 0),
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HArena_"), 0)
  };
  LLVMTypeRef ret_type = LLVMFunctionType(LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParseResult_*"), 0), param_types, 2, 0);
  LLVMValueRef ch = LLVMAddFunction(mod, "ch", ret_type);
  // get the parameter array to use later with h_bits
  LLVMValueRef bits_args[3];
  LLVMGetParams(ch, bits_args);
  bits_args[1] = LLVMConstInt(LLVMInt32Type(), 8, 0);
  bits_args[2] = LLVMConstInt(LLVMInt8Type(), 0, 0);
  LLVMValueRef arena = LLVMGetParam(ch, 1);
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(ch, "ch_entry");
  LLVMBasicBlockRef success = LLVMAppendBasicBlock(ch, "ch_success");
  LLVMBasicBlockRef fail = LLVMAppendBasicBlock(ch, "ch_fail");
  LLVMBasicBlockRef end = LLVMAppendBasicBlock(ch, "ch_end");
  LLVMPositionBuilderAtEnd(builder, entry);
  // %1 = alloca %struct.HParseResult_*, align 8
  LLVMValueRef ret = LLVMBuildAlloca(builder, LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParseResult_*"), 0), "ret");
  // skip %2 through %c because we have these from arguments, and %r because we'll get it later
  // %tok = alloca %struct.HParsedToken_*, align 8
  LLVMValueRef tok = LLVMBuildAlloca(builder, LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParsedToken_*"), 0), "tok");
  // skip next instr through %8
  // %9 = call i64 @h_read_bits(%struct.HInputStream_* %8, i32 8, i8 signext 0)
  LLVMValueRef bits = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "h_read_bits"), bits_args, 3, "h_read_bits");
  // %10 = trunc i64 %9 to i8
  LLVMValueRef r = LLVMBuildTrunc(builder, bits, LLVMInt8Type(), "");
  // %11 = load i8* %c
  // %12 = zext i8 %11 to i32
  // %13 = load i8* %r, align 1
  // %14 = zext i8 %13 to i32
  // %15 = icmp eq i32 %12, %14
  LLVMValueRef icmp = LLVMBuildICmp(builder, LLVMIntEQ, c, r, "c == r");
  // br i1 %15, label %16, label %34
  LLVMBuildCondBr(builder, icmp, success, fail);

  // ; <label>:16 - success case
  LLVMPositionBuilderAtEnd(builder, success);
  // skip %17-%19 because we have the arena as an argument
  LLVMValueRef tok_size = LLVMConstInt(LLVMInt32Type(), sizeof(HParsedToken), 0);
  LLVMValueRef amalloc_args[] = { arena, tok_size };
  // %20 = call noalias i8* @h_arena_malloc(%struct.HArena_* %19, i64 48)
  LLVMValueRef amalloc = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "h_arena_malloc"), amalloc_args, 2, "h_arena_malloc");
  // %21 = bitcast i8* %20 to %struct.HParsedToken_*
  LLVMValueRef cast1 = LLVMBuildBitCast(builder, amalloc, LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParsedToken_"), 0), "");
  // store %struct.HParsedToken_* %21, %struct.HParsedToken_** %tok, align 8
  LLVMBuildStore(builder, cast1, tok);
  // %22 = load %struct.HParsedToken_** %tok, align 8
  // %23 = getelementptr inbounds %struct.HParsedToken_* %22, i32 0, i32 0
  LLVMValueRef bounds1[] = {
    LLVMConstInt(LLVMInt32Type(), 0, 0),
    LLVMConstInt(LLVMInt32Type(), 0, 0)
  };
  LLVMValueRef toktype = LLVMBuildInBoundsGEP(builder, tok, bounds1, 0, "token_type");
  // store i32 8, i32* %23, align 4
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 8, 0), toktype);
  // %24 = load i8* %r, align 1
  // %25 = zext i8 %24 to i64
  // %26 = load %struct.HParsedToken_** %tok, align 8
  // %27 = getelementptr inbounds %struct.HParsedToken_* %26, i32 0, i32 1
  /* LLVMValueRef bounds2[] = { */
  /*   LLVMConstInt(LLVMInt32Type(), 0, 0), */
  /*   LLVMConstInt(LLVMInt32Type(), 0, 1) */
  /* }; */
  /* LLVMValueRef uint = LLVMBuildInBoundsGEP(builder, tok, bounds2, 1, "uint"); */
  // %28 = bitcast %union.anon* %27 to i64*
  //LLVMValueRef cast2 = LLVMBuildBitCast(builder, uint, LLVMInt64Type(), "result");
  LLVMValueRef uint = LLVMConstInt(LLVMInt32Type(), TT_UINT, 0);
  LLVMBuildInsertValue(builder, tok, uint, 0, "uint");
  LLVMBuildInsertValue(builder, tok, r, 1, "r");
  // we already have the arena and the token, so skip to %33
  // %33 = call %struct.HParseResult_* @make_result(%struct.HArena_* %31, %struct.HParsedToken_* %32)
  LLVMValueRef result_args[] = { arena, tok };
  LLVMValueRef mr = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "make_result"), result_args, 2, "make_result");
  // store %struct.HParseResult_* %33, %struct.HParseResult_** %1
  LLVMBuildStore(builder, mr, ret);
  // br label %35
  LLVMBuildBr(builder, end);
  
  // ; <label>:34 - failure case
  LLVMPositionBuilderAtEnd(builder, fail);
  // store %struct.HParseResult* null, %struct.HParseResult_** %1
  LLVMBuildStore(builder, LLVMConstNull(LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParsedToken"), 0)), ret);
  // br label %35
  LLVMBuildBr(builder, end);

  // ; <label>:35
  LLVMPositionBuilderAtEnd(builder, end);
  // %36 = load %struct.HParseResult_** %1
  // ret %struct.HParseResult_* %36
  LLVMBuildRet(builder, ret);
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
