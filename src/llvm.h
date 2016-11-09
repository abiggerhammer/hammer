#ifndef HAMMER_LLVM__H
#define HAMMER_LLVM__H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop

LLVMTypeRef llvm_inputstream, llvm_inputstreamptr, llvm_arena, llvm_arenaptr;
LLVMTypeRef llvm_parsedtoken, llvm_parsedtokenptr, llvm_parseresult, llvm_parseresultptr;

#endif // #ifndef HAMMER_LLVM__H
