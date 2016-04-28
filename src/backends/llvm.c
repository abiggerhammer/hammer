#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include "../internal.h"
#include "../parsers/parser_internal.h"

typedef struct HLLVMParser_ {
  LLVMModuleRef mod;
  LLVMValueRef func;
  LLVMExecutionEngineRef engine;
  LLVMBuilderRef builder;
} HLLVMParser;

void h_llvm_declare_common(LLVMModuleRef mod) {
  LLVMTypeRef readbits_pt[] = {
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HInputStream_"), 0),
    LLVMInt32Type(),
    LLVMInt8Type()
  };
  LLVMTypeRef readbits_ret = LLVMFunctionType(LLVMInt64Type(), readbits_pt, 3, 0);
  LLVMAddFunction(mod, "h_read_bits", readbits_ret);

  LLVMTypeRef amalloc_pt[] = {
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HArena_"), 0),
    LLVMInt32Type()
  };
  LLVMTypeRef amalloc_ret = LLVMFunctionType(LLVMPointerType(LLVMVoidType(), 0), amalloc_pt, 2, 0);
  LLVMAddFunction(mod, "h_arena_malloc", amalloc_ret);

  LLVMTypeRef makeresult_pt[] = {
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HArena_"), 0),
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParsedToken_"), 0)
  };
  LLVMTypeRef makeresult_ret = LLVMFunctionType(LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParseResult_"), 0), makeresult_pt, 2, 0);
  LLVMAddFunction(mod, "make_result", makeresult_ret);
}

int h_llvm_compile(HAllocator* mm__, HParser* parser, const void* params) {
  // Boilerplate to set up a translation unit, aka a module.
  const char* name = params ? (const char*)params : "parse";
  LLVMModuleRef mod = LLVMModuleCreateWithName(name);
  h_llvm_declare_common(mod);
  // Boilerplate to set up the parser function to add to the module. It takes an HInputStream* and
  // returns an HParseResult.
  LLVMTypeRef param_types[] = {
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HInputStream_"), 0),
    LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HArena_"), 0)
  };
  LLVMTypeRef ret_type = LLVMFunctionType(LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "%struct.HParseResult_"), 0), param_types, 2, 0);
  LLVMValueRef parse_func = LLVMAddFunction(mod, name, ret_type);
  // Parse function is now declared; time to define it
  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(parse_func, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);
  // Translate the contents of the children of `parser` into their LLVM instruction equivalents
  if (parser->vtable->llvm(builder, mod, parser->env)) {
    // But first, verification
    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    error = NULL;
    // OK, link that sonofabitch
    LLVMInitializeNativeTarget();
    LLVMLinkInMCJIT();
    LLVMExecutionEngineRef engine = NULL;
    LLVMCreateExecutionEngineForModule(&engine, mod, &error);
    if (error) {
      fprintf(stderr, "error: %s\n", error);
      LLVMDisposeMessage(error);
      return -1;
    }
    // Package up the pointers that comprise the module and stash it in the original HParser
    HLLVMParser *llvm_parser = h_new(HLLVMParser, 1);
    llvm_parser->mod = mod;
    llvm_parser->func = parse_func;
    llvm_parser->engine = engine;
    llvm_parser->builder = builder;
    parser->backend_data = llvm_parser;
    return 0;
  } else {
    return -1;
  }
}

void h_llvm_free(HParser *parser) {
  HLLVMParser *llvm_parser = parser->backend_data;
  LLVMDisposeBuilder(llvm_parser->builder);
  LLVMDisposeExecutionEngine(llvm_parser->engine);
  LLVMDisposeModule(llvm_parser->mod);
}

HParseResult *h_llvm_parse(HAllocator* mm__, const HParser* parser, HInputStream *input_stream) {
  const HLLVMParser *llvm_parser = parser->backend_data;
  LLVMGenericValueRef args[] = { LLVMCreateGenericValueOfPointer(input_stream) };
  LLVMGenericValueRef ret = LLVMRunFunction(llvm_parser->engine, llvm_parser->func, 1, args);
  return (HParseResult*)LLVMGenericValueToPointer(ret);
}

HParserBackendVTable h__llvm_backend_vtable = {
  .compile = h_llvm_compile,
  .parse = h_llvm_parse,
  .free = h_llvm_free
};
