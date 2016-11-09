#include <assert.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "parser_internal.h"
#include "../llvm.h"

struct bits_env {
  uint8_t length;
  uint8_t signedp;
};

static HParseResult* parse_bits(void* env, HParseState *state) {
  struct bits_env *env_ = env;
  HParsedToken *result = a_new(HParsedToken, 1);
  result->token_type = (env_->signedp ? TT_SINT : TT_UINT);
  if (env_->signedp)
    result->sint = h_read_bits(&state->input_stream, env_->length, true);
  else
    result->uint = h_read_bits(&state->input_stream, env_->length, false);
  return make_result(state->arena, result);
}

static bool bits_llvm(LLVMBuilderRef builder, LLVMModuleRef mod, void* env) {
  /*   %result = alloca %struct.HParsedToken_*, align 8 */
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wunused-variable"
  LLVMValueRef result = LLVMBuildAlloca(builder, llvm_parsedtoken, "result");
  #pragma GCC diagnostic pop
  /*   store i8* %env, i8** %1, align 8 */
  /*   store %struct.HParseState_* %state, %struct.HParseState_** %2, align 8 */
  /*   %3 = load i8** %1, align 8 */
  /*   %4 = bitcast i8* %3 to %struct.bits_env* */
  /*   store %struct.bits_env* %4, %struct.bits_env** %env, align 8 */
  /*   %5 = load %struct.HParseState_** %2, align 8 */
  /*   %6 = getelementptr inbounds %struct.HParseState_* %5, i32 0, i32 2 */
  /*   %7 = load %struct.HArena_** %6, align 8 */
  /*   %8 = call noalias i8* @h_arena_malloc(%struct.HArena_* %7, i64 48) */
  /*   %9 = bitcast i8* %8 to %struct.HParsedToken_* */
  /*   store %struct.HParsedToken_* %9, %struct.HParsedToken_** %result, align 8 */
  /*   %10 = load %struct.bits_env** %env_, align 8 */
  /*   %11 = getelementptr inbounds %struct.bits_env* %10, i32 0, i32 1 */
  /*   %12 = load i8* %11, align 1 */
  /*   %13 = zext i8 %12 to i32 */
  /*   %14 = icmp ne i32 %13, 0 */
  /*   %15 = select i1 %14, i32 4, i32 8 */
  /*   %16 = load %struct.HParsedToken_** %result, align 8 */
  /*   %17 = getelementptr inbounds %struct.HParsedToken_* %16, i32 0, i32 0 */
  /*   store i32 %15, i32* %17, align 4 */
  /*   %18 = load %struct.bits_env** %env_, align 8 */
  /*   %19 = getelementptr inbounds %struct.bits_env* %18, i32 0, i32 1 */
  /*   %20 = load i8* %19, align 1 */
  /*   %21 = icmp ne i8 %20, 0 */
  /*   br i1 %21, label %22, label %33 */

  /* ; <label>:22                                      ; preds = %0 */
  /*   %23 = load %struct.HParseState_** %2, align 8 */
  /*   %24 = getelementptr inbounds %struct.HParseState_* %23, i32 0, i32 1 */
  /*   %25 = load %struct.bits_env** %env_, align 8 */
  /*   %26 = getelementptr inbounds %struct.bits_env* %25, i32 0, i32 0 */
  /*   %27 = load i8* %26, align 1 */
  /*   %28 = zext i8 %27 to i32 */
  /*   %29 = call i64 @h_read_bits(%struct.HInputStream_* %24, i32 %28, i8 signext 1) */
  /*   %30 = load %struct.HParsedToken_** %result, align 8 */
  /*   %31 = getelementptr inbounds %struct.HParsedToken_* %30, i32 0, i32 1 */
  /*   %32 = bitcast %union.anon* %31 to i64* */
  /*   store i64 %29, i64* %32, align 8 */
  /*   br label %44 */

  /* ; <label>:33                                      ; preds = %0 */
  /*   %34 = load %struct.HParseState_** %2, align 8 */
  /*   %35 = getelementptr inbounds %struct.HParseState_* %34, i32 0, i32 1 */
  /*   %36 = load %struct.bits_env** %env_, align 8 */
  /*   %37 = getelementptr inbounds %struct.bits_env* %36, i32 0, i32 0 */
  /*   %38 = load i8* %37, align 1 */
  /*   %39 = zext i8 %38 to i32 */
  /*   %40 = call i64 @h_read_bits(%struct.HInputStream_* %35, i32 %39, i8 signext 0) */
  /*   %41 = load %struct.HParsedToken_** %result, align 8 */
  /*   %42 = getelementptr inbounds %struct.HParsedToken_* %41, i32 0, i32 1 */
  /*   %43 = bitcast %union.anon* %42 to i64* */
  /*   store i64 %40, i64* %43, align 8 */
  /*   br label %44 */
  
  /* ; <label>:44                                      ; preds = %33, %22 */
  /*   %45 = load %struct.HParseState_** %2, align 8 */
  /*   %46 = getelementptr inbounds %struct.HParseState_* %45, i32 0, i32 2 */
  /*   %47 = load %struct.HArena_** %46, align 8 */
  /*   %48 = load %struct.HParsedToken_** %result, align 8 */
  /*   %49 = call %struct.HParseResult_* @make_result(%struct.HArena_* %47, %struct.HParsedToken_* %48) */
  /*   ret %struct.HParseResult_* %49 */
  return true;
}

static HParsedToken *reshape_bits(const HParseResult *p, void* signedp_p) {
  // signedp == NULL iff unsigned
  bool signedp = (signedp_p != NULL);
  // XXX works only for whole bytes
  // XXX assumes big-endian
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);

  HCountedArray *seq = p->ast->seq;
  HParsedToken *ret = h_arena_malloc(p->arena, sizeof(HParsedToken));
  ret->token_type = TT_UINT;

  if(signedp && (seq->elements[0]->uint & 128))
    ret->uint = -1; // all ones

  for(size_t i=0; i<seq->used; i++) {
    HParsedToken *t = seq->elements[i];
    assert(t->token_type == TT_UINT);

    ret->uint <<= 8;
    ret->uint |= t->uint & 0xFF;
  }

  if(signedp) {
    ret->token_type = TT_SINT;
    ret->sint = ret->uint;
  }

  return ret;
}

static void desugar_bits(HAllocator *mm__, HCFStack *stk__, void *env) {
  struct bits_env *bits = (struct bits_env*)env;
  assert (0 == bits->length % 8);

  HCharset match_all = new_charset(mm__);
  for (int i = 0; i < 256; i++)
    charset_set(match_all, i, 1);

  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      size_t n = bits->length/8;
      for (size_t i=0; i<n; ++i) {
	HCFS_ADD_CHARSET(match_all);
      }
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = reshape_bits;
    HCFS_THIS_CHOICE->user_data = bits->signedp ? HCFS_THIS_CHOICE : NULL; // HCFS_THIS_CHOICE is an arbitrary non-null pointer
    
  } HCFS_END_CHOICE();
}

static bool h_svm_action_bits(HArena *arena, HSVMContext *ctx, void* env) {
  // BUG: relies un undefined behaviour: int64_t is a signed uint64_t; not necessarily true on 32-bit
  struct bits_env *env_ = env;
  HParsedToken *top = ctx->stack[ctx->stack_count-1];
  assert(top->token_type == TT_BYTES);
  uint64_t res = 0;
  for (size_t i = 0; i < top->bytes.len; i++)
    res = (res << 8) | top->bytes.token[i];   // TODO: Handle other endiannesses.
  uint64_t msb = (env_->signedp ? 1LL:0) << (top->bytes.len * 8 - 1);
  res = (res ^ msb) - msb;
  top->uint = res; // possibly cast to signed through union
  top->token_type = (env_->signedp ? TT_SINT : TT_UINT);
  return true;
}

static bool bits_ctrvm(HRVMProg *prog, void* env) {
  struct bits_env *env_ = (struct bits_env*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (size_t i=0; i < (env_->length/8); ++i) { // FUTURE: when we can handle non-byte-aligned, the env_->length/8 part will be different
    h_rvm_insert_insn(prog, RVM_MATCH, 0xFF00);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
  }
  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_bits, env));
  return true;
}

static const HParserVtable bits_vt = {
  .parse = parse_bits,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_bits,
  .compile_to_rvm = bits_ctrvm,
  .llvm = bits_llvm,
  .higher = false,
};

HParser* h_bits(size_t len, bool sign) {
  return h_bits__m(&system_allocator, len, sign);
}
HParser* h_bits__m(HAllocator* mm__, size_t len, bool sign) {
  struct bits_env *env = h_new(struct bits_env, 1);
  env->length = len;
  env->signedp = sign;
  return h_new_parser(mm__, &bits_vt, env);
}

#define SIZED_BITS(name_pre, len, signedp) \
  HParser* h_##name_pre##len () {				\
    return h_bits__m(&system_allocator, len, signedp);		\
  }								\
  HParser* h_##name_pre##len##__m(HAllocator* mm__) {	\
    return h_bits__m(mm__, len, signedp);			\
  }
SIZED_BITS(int, 8, true)
SIZED_BITS(int, 16, true)
SIZED_BITS(int, 32, true)
SIZED_BITS(int, 64, true)
SIZED_BITS(uint, 8, false)
SIZED_BITS(uint, 16, false)
SIZED_BITS(uint, 32, false)
SIZED_BITS(uint, 64, false)
