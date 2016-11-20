#ifndef HAMMER_LLVM__H
#define HAMMER_LLVM__H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop

LLVMTypeRef llvm_inputstream, llvm_inputstreamptr, llvm_arena, llvm_arenaptr;
LLVMTypeRef llvm_parsedtoken, llvm_parsedtokenptr, llvm_parseresult, llvm_parseresultptr;

void h_llvm_make_charset_membership_test(HAllocator* mm__,
                                         LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                         LLVMValueRef r, HCharset cs,
                                         LLVMBasicBlockRef yes, LLVMBasicBlockRef no);
void h_llvm_make_tt_suint(LLVMModuleRef mod, LLVMBuilderRef builder,
                          LLVMValueRef stream, LLVMValueRef arena, 
                          LLVMValueRef r, LLVMValueRef *mr_out);

#endif // #ifndef HAMMER_LLVM__H
