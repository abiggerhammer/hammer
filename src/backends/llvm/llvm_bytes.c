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
 * Construct LLVM IR to allocate a token of type TT_BYTES with a compile-time
 * constant value
 *
 * Parameters:
 *  - ctxt [in]: an HLLVMParserCompileContext
 *  - bytes [in]: an array of bytes
 *  - len [in]: size of bytes
 *  - mr_out [out]: the return value from make_result()
 */

void h_llvm_make_tt_bytes_fixed(HLLVMParserCompileContext *ctxt,
                                const uint8_t *bytes, size_t len,
                                LLVMValueRef *mr_out) {
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
   * tok->token_type = TT_BYTES;
   */
  LLVMValueRef toktype = LLVMBuildStructGEP(ctxt->builder, tok, 0, "token_type");
  LLVMBuildStore(ctxt->builder, LLVMConstInt(LLVMInt32Type(), TT_BYTES, 0), toktype);

  /*
   * XXX the way LLVM handles unions is batshit insane and forces IR writers
   * to figure out which element of the union is largest just to declare the
   * type, and then get all the alignments right - in effect, manually crufting
   * up something compatible with their C compiler's ABI.  This is not so much
   * a portability bug as a portability bug queen with a bone-penetrating
   * ovipositor for laying her eggs in one's brain.
   *
   * The sole saving grace here is that the limited number of platforms LLVM
   * can JIT on make it conceivable I may get this right for the cases that come
   * up in practice if not for the general case.  If it breaks horribly, the
   * slightly slower but safe option is to implement a function to set the
   * relevant union fields from its arguments in C and build a call to it.
   *
   * The equivalent C that prompted this rant is quite depressingly simple:
   *
   * tok->bytes.token = bytes;
   * tok->bytes.len = len;
   */

  LLVMValueRef hbytes_gep_tmp =
    LLVMBuildStructGEP(ctxt->builder, tok, 1, "tok_union");
  LLVMValueRef hbytes_gep = LLVMBuildBitCast(ctxt->builder, hbytes_gep_tmp,
      ctxt->llvm_hbytesptr, "hbytes");
  LLVMValueRef hbytes_token_gep =
    LLVMBuildStructGEP(ctxt->builder, hbytes_gep, 0, "hbytes_token");
  /*
   * We have to do this silly (uintptr_t) / LLVMConstIntToPtr() dance because
   * LLVM doesn't seem to offer any way to construct a compile-time pointer
   * constant other than NULL directly.
   */
  LLVMBuildStore(ctxt->builder,
      LLVMConstIntToPtr(LLVMConstInt(ctxt->llvm_intptr_t, (uintptr_t)bytes, 0),
        LLVMPointerType(LLVMInt8Type(), 0)),
      hbytes_token_gep);
  LLVMValueRef hbytes_len_gep =
    LLVMBuildStructGEP(ctxt->builder, hbytes_gep, 1, "hbytes_len");
  LLVMBuildStore(ctxt->builder, LLVMConstInt(ctxt->llvm_size_t, len, 0), hbytes_len_gep);

  /*
   * Now call make_result()
   */
  LLVMValueRef result_args[] = { ctxt->arena, tok };
  LLVMValueRef mr = LLVMBuildCall(ctxt->builder,
      LLVMGetNamedFunction(ctxt->mod, "make_result"),
      result_args, 2, "make_result");

  *mr_out = mr;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */
