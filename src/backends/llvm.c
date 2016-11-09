#include <llvm-c/Analysis.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include <llvm-c/ExecutionEngine.h>
#include "../internal.h"
#include "../llvm.h"
#include "../parsers/parser_internal.h"

typedef struct HLLVMParser_ {
  LLVMModuleRef mod;
  LLVMValueRef func;
  LLVMExecutionEngineRef engine;
  LLVMBuilderRef builder;
} HLLVMParser;

void h_llvm_declare_common(LLVMModuleRef mod) {
  llvm_inputstream = LLVMPointerType(LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HInputStream_"), 0);
  llvm_arena = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HArena_");
  llvm_arenaptr = LLVMPointerType(llvm_arena, 0);
  llvm_parsedtoken = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HParsedToken_");
  LLVMTypeRef llvm_parsedtoken_struct_types[] = {
    LLVMInt32Type(), // actually an enum value
    LLVMInt64Type(), // actually this is a union; the largest thing in it is 64 bits
    LLVMInt64Type(), // FIXME sizeof(size_t) will be 32 bits on 32-bit platforms
    LLVMInt64Type(), // FIXME ditto
    LLVMInt8Type()
  };
  LLVMStructSetBody(llvm_parsedtoken, llvm_parsedtoken_struct_types, 5, 0);
  llvm_parsedtokenptr = LLVMPointerType(llvm_parsedtoken, 0);
  llvm_parseresult = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HParseResult_");
  LLVMTypeRef llvm_parseresult_struct_types[] = {
    llvm_parsedtokenptr,
    LLVMInt64Type(),
    llvm_arenaptr
  };
  LLVMStructSetBody(llvm_parseresult, llvm_parseresult_struct_types, 3, 0);
  llvm_parseresultptr = LLVMPointerType(llvm_parseresult, 0);
  LLVMTypeRef readbits_pt[] = {
    llvm_inputstream,
    LLVMInt32Type(),
    LLVMInt8Type()
  };
  LLVMTypeRef readbits_ret = LLVMFunctionType(LLVMInt64Type(), readbits_pt, 3, 0);
  LLVMAddFunction(mod, "h_read_bits", readbits_ret);

  LLVMTypeRef amalloc_pt[] = {
    llvm_arena,
    LLVMInt32Type()
  };
  LLVMTypeRef amalloc_ret = LLVMFunctionType(LLVMPointerType(LLVMVoidType(), 0), amalloc_pt, 2, 0);
  LLVMAddFunction(mod, "h_arena_malloc", amalloc_ret);

  LLVMTypeRef makeresult_pt[] = {
    llvm_arena,
    llvm_parsedtokenptr
  };
  LLVMTypeRef makeresult_ret = LLVMFunctionType(llvm_parseresultptr, makeresult_pt, 2, 0);
  LLVMAddFunction(mod, "make_result", makeresult_ret);
  char* dump = LLVMPrintModuleToString(mod);
  fprintf(stderr, "\n\n%s\n\n", dump);
}

int h_llvm_compile(HAllocator* mm__, HParser* parser, const void* params) {
  // Boilerplate to set up a translation unit, aka a module.
  const char* name = params ? (const char*)params : "parse";
  LLVMModuleRef mod = LLVMModuleCreateWithName(name);
  h_llvm_declare_common(mod);
  // Boilerplate to set up the parser function to add to the module. It takes an HInputStream* and
  // returns an HParseResult.
  LLVMTypeRef param_types[] = {
    llvm_inputstream,
    llvm_arena
  };
  LLVMTypeRef ret_type = LLVMFunctionType(llvm_parseresultptr, param_types, 2, 0);
  LLVMValueRef parse_func = LLVMAddFunction(mod, name, ret_type);
  // Parse function is now declared; time to define itt
  LLVMBuilderRef builder = LLVMCreateBuilder();
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
