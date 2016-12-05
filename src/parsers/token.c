#include <assert.h>
#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif
#include "parser_internal.h"

typedef struct {
  uint8_t *str;
  uint8_t len;
} HToken;

static HParseResult* parse_token(void *env, HParseState *state) {
  HToken *t = (HToken*)env;
  for (int i=0; i<t->len; ++i) {
    uint8_t chr = (uint8_t)h_read_bits(&state->input_stream, 8, false);
    if (t->str[i] != chr) {
      return NULL;
    }
  }
  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_BYTES; tok->bytes.token = t->str; tok->bytes.len = t->len;
  return make_result(state->arena, tok);
}

static HParsedToken *reshape_token(const HParseResult *p, void* user_data) {
  // fetch sequence of uints from p
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  HCountedArray *seq = p->ast->seq;

  // extract byte string
  uint8_t *arr = h_arena_malloc(p->arena, seq->used);
  size_t i;
  for(i=0; i<seq->used; i++) {
    HParsedToken *t = seq->elements[i];
    assert(t->token_type == TT_UINT);
    arr[i] = t->uint;
  }

  // create result token
  HParsedToken *tok = h_arena_malloc(p->arena, sizeof(HParsedToken));
  tok->token_type = TT_BYTES;
  tok->bytes.len = seq->used;
  tok->bytes.token = arr;

  return tok;
}

static void desugar_token(HAllocator *mm__, HCFStack *stk__, void *env) {
  HToken *tok = (HToken*)env;
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      for (size_t i = 0; i < tok->len; i++)
	HCFS_ADD_CHAR(tok->str[i]);
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = reshape_token;
    HCFS_THIS_CHOICE->user_data = NULL;
  } HCFS_END_CHOICE();
}

static bool token_ctrvm(HRVMProg *prog, void *env) {
  HToken *t = (HToken*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (int i=0; i<t->len; ++i) {
    h_rvm_insert_insn(prog, RVM_MATCH, t->str[i] | t->str[i] << 8);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
  }
  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  return true;
}

#ifdef HAMMER_LLVM_BACKEND

/*
 * Emit LLVM IR to recognize a token by comparing it to a string stored in
 * the LLVM module globals.  We use this for longer tokens.
 */

static bool token_llvm_with_global(HLLVMParserCompileContext *ctxt, HToken *t) {
  /* TODO */
  return true;
}

/*
 * Emit LLVM IR to recognize a token by sequentially checking each character;
 * suitable for short tokens.  This also handles the zero-length token case.
 */

static bool token_llvm_with_sequential_comparisons(HLLVMParserCompileContext *ctxt, HToken *t) {
  HAllocator *mm__;
  LLVMValueRef bits, r, c, icmp, mr, rv;
  LLVMValueRef bits_args[3];
  LLVMBasicBlockRef entry, success, end, next_char;
  char name[64];
  int i;

  /* Get allocator ready */
  mm__ = ctxt->mm__;

  /* Set up basic blocks: entry, success and exit branches */
  entry = LLVMAppendBasicBlock(ctxt->func, "tok_seq_entry");
  success = LLVMAppendBasicBlock(ctxt->func, "tok_seq_success");
  end = LLVMAppendBasicBlock(ctxt->func, "tok_seq_end");

  /* Branch to entry block */
  LLVMBuildBr(ctxt->builder, entry);
  LLVMPositionBuilderAtEnd(ctxt->builder, entry);

  /* Basic block refs for the phi later */
  LLVMBasicBlockRef *bbs_into_phi = h_new(LLVMBasicBlockRef, 1 + t->len);
  LLVMValueRef *values_into_phi = h_new(LLVMValueRef, 1 + t->len);

  /* For each char of token... */
  bits_args[0] = ctxt->stream;
  bits_args[1] = LLVMConstInt(LLVMInt32Type(), 8, 0);
  bits_args[2] = LLVMConstInt(LLVMInt8Type(), 0, 0);
  /* Track the current basic block */
  LLVMBasicBlockRef curr_char = entry;
  for (i = 0; i < t->len; ++i) {
    /* Read a char */
    bits = LLVMBuildCall(ctxt->builder,
        LLVMGetNamedFunction(ctxt->mod, "h_read_bits"), bits_args, 3, "read_bits");
    /* Clamp to i8 */
    r = LLVMBuildTrunc(ctxt->builder, bits, LLVMInt8Type(), "");
    /* Comparison */
    c = LLVMConstInt(LLVMInt8Type(), t->str[i], 0);
    snprintf(name, 64, "t->str[%d] == r", i);
    icmp = LLVMBuildICmp(ctxt->builder, LLVMIntEQ, c, r, name);
    /* Next basic block */
    snprintf(name, 64, "tok_matched_%d", i);
    next_char = LLVMAppendBasicBlock(ctxt->func, name);
    /* Conditional branch */
    LLVMBuildCondBr(ctxt->builder, icmp, next_char, end);
    /* Fill in our row in the phi tables */
    bbs_into_phi[1 + i] = curr_char;
    values_into_phi[1 + i] = LLVMConstNull(ctxt->llvm_parseresultptr);
    /* Start from next_char */
    LLVMPositionBuilderAtEnd(ctxt->builder, next_char);
    /* Update the current basic block */
    curr_char = next_char;
  }

  /* If we got here, accept the token */
  LLVMBuildBr(ctxt->builder, success);

  /* Success block: make a token */
  LLVMPositionBuilderAtEnd(ctxt->builder, success);
  h_llvm_make_tt_bytes_fixed(ctxt, t->str, t->len, &mr);
  /* Fill in our row in the phi tables */
  bbs_into_phi[0] = success;
  values_into_phi[0] = mr;
  /* Branch to end so we can return the token */
  LLVMBuildBr(ctxt->builder, end);

  /* End block: return a token if we made one */
  LLVMPositionBuilderAtEnd(ctxt->builder, end);
  /* phi the token or a null depending on where we came from */
  rv = LLVMBuildPhi(ctxt->builder, ctxt->llvm_parseresultptr, "rv");
  LLVMAddIncoming(rv, values_into_phi, bbs_into_phi, 1 + t->len);
  /* Free the stuff we allocated to build the phi */
  h_free(bbs_into_phi);
  h_free(values_into_phi);
  /* Return it */
  LLVMBuildRet(ctxt->builder, rv);

  return true;
}

#define TOKEN_LENGTH_USE_GLOBAL_CUTOFF 4

static bool token_llvm(HLLVMParserCompileContext *ctxt, void* env) {
  HToken *t;
  if (!ctxt) return false;

  /* Get the token */
  t = (HToken *)env;
  /*
   * Check its length; we have two possible code-generation strategies
   * here: treat it like chars sequentially and emit a series of read/
   * tests, or put the string in the LLVM module globals and compare
   * in a loop.  Use the former for very short strings and the latter
   * for longer ones.
   *
   * XXX Like with charsets, we should also think about memoizing these
   * for recurring strings.
   */
  if (t->len > TOKEN_LENGTH_USE_GLOBAL_CUTOFF &&
      t->len > 0) {
    return token_llvm_with_global(ctxt, t);
  } else {
    return token_llvm_with_sequential_comparisons(ctxt, t);
  }
}

#endif /* defined(HAMMER_LLVM_BACKEND) */

const HParserVtable token_vt = {
  .parse = parse_token,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_token,
  .compile_to_rvm = token_ctrvm,
#ifdef HAMMER_LLVM_BACKEND
  .llvm = token_llvm,
#endif
  .higher = false,
};

HParser* h_token(const uint8_t *str, const size_t len) {
  return h_token__m(&system_allocator, str, len);
}
HParser* h_token__m(HAllocator* mm__, const uint8_t *str, const size_t len) { 
  HToken *t = h_new(HToken, 1);
  uint8_t *str_cpy = h_new(uint8_t, len);
  memcpy(str_cpy, str, len);
  t->str = str_cpy, t->len = len;
  return h_new_parser(mm__, &token_vt, t);
}
