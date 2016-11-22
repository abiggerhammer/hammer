#ifdef HAMMER_LLVM_BACKEND

#include <llvm-c/Analysis.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include <llvm-c/ExecutionEngine.h>
#include "../../internal.h"
#include "llvm.h"

/*
 * Construct LLVM IR to allocate a token of type TT_SINT or TT_UINT
 *
 * Parameters:
 *  - mod [in]: an LLVMModuleRef
 *  - builder [in]: an LLVMBuilderRef, positioned appropriately
 *  - stream [in]: a value ref to an llvm_inputstreamptr, for the input stream
 *  - arena [in]: a value ref to an llvm_arenaptr to be used for the malloc
 *  - r [in]: a value ref to the value to be used to this token
 *  - mr_out [out]: the return value from make_result()
 *
 * TODO actually support TT_SINT, inputs other than 8 bit
 */

void h_llvm_make_tt_suint(HLLVMParserCompileContext *ctxt,
                          LLVMValueRef r, LLVMValueRef *mr_out) {
  /* Set up call to h_arena_malloc() for a new HParsedToken */
  LLVMValueRef tok_size = LLVMConstInt(LLVMInt32Type(), sizeof(HParsedToken), 0);
  LLVMValueRef amalloc_args[] = { ctxt->arena, tok_size };
  /* %h_arena_malloc = call void* @h_arena_malloc(%struct.HArena_.1* %1, i32 48) */
  LLVMValueRef amalloc = LLVMBuildCall(ctxt->builder,
      LLVMGetNamedFunction(ctxt->mod, "h_arena_malloc"),
      amalloc_args, 2, "h_arena_malloc");
  /* %tok = bitcast void* %h_arena_malloc to %struct.HParsedToken_.2* */
  LLVMValueRef tok = LLVMBuildBitCast(ctxt->builder, amalloc, ctxt->llvm_parsedtokenptr, "tok");

  /*
   * tok->token_type = TT_UINT;
   *
   * %token_type = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 0
   *
   * TODO if we handle TT_SINT too, adjust here and the zero-ext below
   */
  LLVMValueRef toktype = LLVMBuildStructGEP(ctxt->builder, tok, 0, "token_type");
  /* store i32 8, i32* %token_type */
  LLVMBuildStore(ctxt->builder, LLVMConstInt(LLVMInt32Type(), 8, 0), toktype);

  /*
   * tok->uint = r;
   *
   * %token_data = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 1
   */
  LLVMValueRef tokdata = LLVMBuildStructGEP(ctxt->builder, tok, 1, "token_data");
  /*
   * TODO
   *
   * This is where we'll need to adjust to handle other types (sign vs. zero extend, omit extend if
   * r is 64-bit already
   */
  LLVMBuildStore(ctxt->builder, LLVMBuildZExt(ctxt->builder, r, LLVMInt64Type(), "r"), tokdata);
  /*
   * Store the index from the stream into the token
   */
  /* %t_index = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 2 */
  LLVMValueRef tokindex = LLVMBuildStructGEP(ctxt->builder, tok, 2, "t_index");
  /* %s_index = getelementptr inbounds %struct.HInputStream_.0, %struct.HInputStream_.0* %0, i32 0, i32 2 */
  LLVMValueRef streamindex = LLVMBuildStructGEP(ctxt->builder, ctxt->stream, 2, "s_index");
  /* %4 = load i64, i64* %s_index */
  /* store i64 %4, i64* %t_index */
  LLVMBuildStore(ctxt->builder, LLVMBuildLoad(ctxt->builder, streamindex, ""), tokindex);
  /* Store the bit length into the token */
  LLVMValueRef tokbitlen = LLVMBuildStructGEP(ctxt->builder, tok, 3, "bit_length");
  /* TODO handle multiple bit lengths */
  LLVMBuildStore(ctxt->builder, LLVMConstInt(LLVMInt64Type(), 8, 0), tokbitlen);

  /*
   * Now call make_result()
   *
   * %make_result = call %struct.HParseResult_.3* @make_result(%struct.HArena_.1* %1, %struct.HParsedToken_.2* %3)
   */
  LLVMValueRef result_args[] = { ctxt->arena, tok };
  LLVMValueRef mr = LLVMBuildCall(ctxt->builder,
      LLVMGetNamedFunction(ctxt->mod, "make_result"),
      result_args, 2, "make_result");

  *mr_out = mr;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */
