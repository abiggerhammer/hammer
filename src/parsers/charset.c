#include <assert.h>
#include <string.h>
#include "../internal.h"
#ifdef HAMMER_LLVM_BACKEND
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include "../backends/llvm/llvm.h"
#endif /* defined(HAMMER_LLVM_BACKEND) */
#include "parser_internal.h"

static HParseResult* parse_charset(void *env, HParseState *state) {
  uint8_t in = h_read_bits(&state->input_stream, 8, false);
  HCharset cs = (HCharset)env;

  if (charset_isset(cs, in)) {
    HParsedToken *tok = a_new(HParsedToken, 1);
    tok->token_type = TT_UINT; tok->uint = in;
    return make_result(state->arena, tok);    
  } else
    return NULL;
}

static void desugar_charset(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_ADD_CHARSET( (HCharset)env );
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

// FUTURE: this is horribly inefficient
static bool cs_ctrvm(HRVMProg *prog, void *env) {
  HCharset cs = (HCharset)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);

  uint16_t start = h_rvm_get_ip(prog);

  uint8_t range_start = 0;
  bool collecting = false;
  for (size_t i=0; i<257; ++i) {
    // Position 256 is only there so that every included character has
    // a non-included character after it.
    if (i < 256 && charset_isset(cs, i)) {
      if (!collecting) {
	collecting = true;
	range_start = i;
      }
    } else {
      if (collecting) {
	collecting = false;
	uint16_t insn = h_rvm_insert_insn(prog, RVM_FORK, 0);
	h_rvm_insert_insn(prog, RVM_MATCH, range_start | (i-1) << 8);
	h_rvm_insert_insn(prog, RVM_GOTO, 0);
	h_rvm_patch_arg(prog, insn, h_rvm_get_ip(prog));
      }
    }
  }
  h_rvm_insert_insn(prog, RVM_MATCH, 0x00FF);
  uint16_t jump = h_rvm_insert_insn(prog, RVM_STEP, 0);
  for (size_t i=start; i<jump; ++i) {
    if (RVM_GOTO == prog->insns[i].op)
      h_rvm_patch_arg(prog, i, jump);
  }

  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_ch, env));
  return true;
}

#ifdef HAMMER_LLVM_BACKEND

static bool cs_llvm(HAllocator *mm__, LLVMBuilderRef builder, LLVMValueRef func,
                    LLVMModuleRef mod, void* env) {
  /*
   * LLVM to build a function to parse a charset; the args are a stream and an
   * arena.
   */

  LLVMValueRef stream = LLVMGetFirstParam(func);
  stream = LLVMBuildBitCast(builder, stream, llvm_inputstreamptr, "stream");
  LLVMValueRef arena = LLVMGetLastParam(func);

  /* Set up our basic blocks */
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(func, "cs_entry");
  LLVMBasicBlockRef success = LLVMAppendBasicBlock(func, "cs_success");
  LLVMBasicBlockRef fail = LLVMAppendBasicBlock(func, "cs_fail");
  LLVMBasicBlockRef end = LLVMAppendBasicBlock(func, "cs_end");

  /* Basic block: entry */
  LLVMPositionBuilderAtEnd(builder, entry);
  /* First we read the char */
  LLVMValueRef bits_args[3];
  bits_args[0] = stream;
  bits_args[1] = LLVMConstInt(LLVMInt32Type(), 8, 0);
  bits_args[2] = LLVMConstInt(LLVMInt8Type(), 0, 0);
  LLVMValueRef bits = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "h_read_bits"), bits_args, 3, "read_bits");
  LLVMValueRef r = LLVMBuildTrunc(builder, bits, LLVMInt8Type(), ""); // TODO Necessary? (same question in ch_llvm())

  /* We have a char, need to check if it's in the charset */
  HCharset cs = (HCharset)env;
  /* Branch to either success or end, conditional on whether r is in cs */
  h_llvm_make_charset_membership_test(mm__, mod, func, builder, r, cs, success, fail);

  /* Basic block: success */
  LLVMPositionBuilderAtEnd(builder, success);

  LLVMValueRef mr;
  h_llvm_make_tt_suint(mod, builder, stream, arena, r, &mr);

  /* br label %ch_end */
  LLVMBuildBr(builder, end);

  /* Basic block: fail */
  LLVMPositionBuilderAtEnd(builder, fail);
  /*
   * We just branch straight to end; this exists so that the phi node in 
   * end knows where all the incoming edges are from, rather than needing
   * some basic block constructed in h_llvm_make_charset_membership_test()
   */
  LLVMBuildBr(builder, end);

  /* Basic block: end */
  LLVMPositionBuilderAtEnd(builder, end);
  // %rv = phi %struct.HParseResult_.3* [ %make_result, %ch_success ], [ null, %ch_entry ]
  LLVMValueRef rv = LLVMBuildPhi(builder, llvm_parseresultptr, "rv");
  LLVMBasicBlockRef rv_phi_incoming_blocks[] = {
    success,
    fail
  };
  LLVMValueRef rv_phi_incoming_values[] = {
    mr,
    LLVMConstNull(llvm_parseresultptr)
  };
  LLVMAddIncoming(rv, rv_phi_incoming_values, rv_phi_incoming_blocks, 2);
  // ret %struct.HParseResult_.3* %rv
  LLVMBuildRet(builder, rv);

  return true;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */

static const HParserVtable charset_vt = {
  .parse = parse_charset,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_charset,
  .compile_to_rvm = cs_ctrvm,
#ifdef HAMMER_LLVM_BACKEND
  .llvm = cs_llvm,
#endif
  .higher = false,
};

HParser* h_ch_range(const uint8_t lower, const uint8_t upper) {
  return h_ch_range__m(&system_allocator, lower, upper);
}
HParser* h_ch_range__m(HAllocator* mm__, const uint8_t lower, const uint8_t upper) {
  HCharset cs = new_charset(mm__);
  for (int i = 0; i < 256; i++)
    charset_set(cs, i, (lower <= i) && (i <= upper));
  return h_new_parser(mm__, &charset_vt, cs);
}


static HParser* h_in_or_not__m(HAllocator* mm__, const uint8_t *options, size_t count, int val) {
  HCharset cs = new_charset(mm__);
  for (size_t i = 0; i < 256; i++)
    charset_set(cs, i, 1-val);
  for (size_t i = 0; i < count; i++)
    charset_set(cs, options[i], val);

  return h_new_parser(mm__, &charset_vt, cs);
}

HParser* h_in(const uint8_t *options, size_t count) {
  return h_in_or_not__m(&system_allocator, options, count, 1);
}

HParser* h_in__m(HAllocator* mm__, const uint8_t *options, size_t count) {
  return h_in_or_not__m(mm__, options, count, 1);
}

HParser* h_not_in(const uint8_t *options, size_t count) {
  return h_in_or_not__m(&system_allocator, options, count, 0);
}

HParser* h_not_in__m(HAllocator* mm__, const uint8_t *options, size_t count) {
  return h_in_or_not__m(mm__, options, count, 0);
}

