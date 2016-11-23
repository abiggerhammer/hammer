#include <assert.h>
#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif
#include "parser_internal.h"

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

#ifdef HAMMER_LLVM_BACKEND

static bool bits_llvm(HLLVMParserCompileContext *ctxt, void* env) {
  /* Emit LLVM IR to parse ((struct bits_env *)env)->length bits */

  if (!ctxt) return false;

  struct bits_env *env_ = env;
  /* Error out on unsupported length */
  if (env_->length > 64 || env_->length == 0) return false;
  /* Set up params for call to h_read_bits */
  LLVMValueRef bits_args[3];
  bits_args[0] = ctxt->stream;
  bits_args[1] = LLVMConstInt(LLVMInt32Type(), env_->length, 0);
  bits_args[2] = LLVMConstInt(LLVMInt8Type(), env_->signedp ? 1 : 0, 0);

  /* Set up basic blocks: entry, success and failure branches, then exit */
  LLVMBasicBlockRef bits_bb = LLVMAppendBasicBlock(ctxt->func, "bits");

  /* Basic block: entry */
  LLVMBuildBr(ctxt->builder, bits_bb);
  LLVMPositionBuilderAtEnd(ctxt->builder, bits_bb);

  /* Call to h_read_bits() */
  // %read_bits = call i64 @h_read_bits(%struct.HInputStream_* %8, i32 env_->length, i8 signext env_->signedp)
  LLVMValueRef bits = LLVMBuildCall(ctxt->builder,
      LLVMGetNamedFunction(ctxt->mod, "h_read_bits"), bits_args, 3, "read_bits");

  /* Make an HParseResult out of it */
  LLVMValueRef mr;
  h_llvm_make_tt_suint(ctxt, env_->length, env_->signedp, bits, &mr);

  /* Return mr */
  LLVMBuildRet(ctxt->builder, mr);

  return true;
}

#endif

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
#ifdef HAMMER_LLVM_BACKEND
  .llvm = bits_llvm,
#endif
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
