#ifdef HAMMER_LLVM_BACKEND

#ifndef HAMMER_LLVM__H
#define HAMMER_LLVM__H

#include "../../internal.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop

/* The typedef is in internal.h */

struct HLLVMParserCompileContext_ {
  /* Allocator */
  HAllocator* mm__;
  /* Module/function/builder */
  LLVMModuleRef mod;
  LLVMValueRef func;
  LLVMBuilderRef builder;
  /* Typerefs */
  LLVMTypeRef llvm_inputstream;
  LLVMTypeRef llvm_inputstreamptr;
  LLVMTypeRef llvm_arena;
  LLVMTypeRef llvm_arenaptr;
  LLVMTypeRef llvm_parsedtoken;
  LLVMTypeRef llvm_parsedtokenptr;
  LLVMTypeRef llvm_parseresult;
  LLVMTypeRef llvm_parseresultptr;
  /* Set up in function preamble */
  LLVMValueRef stream;
  LLVMValueRef arena;
};

bool h_llvm_make_charset_membership_test(HLLVMParserCompileContext *ctxt,
                                         LLVMValueRef r, HCharset cs,
                                         LLVMBasicBlockRef yes, LLVMBasicBlockRef no);
void h_llvm_make_tt_suint(HLLVMParserCompileContext *ctxt,
                          uint8_t length, uint8_t signedp,
                          LLVMValueRef r, LLVMValueRef *mr_out);

#endif // #ifndef HAMMER_LLVM__H

#endif /* defined(HAMMER_LLVM_BACKEND) */
