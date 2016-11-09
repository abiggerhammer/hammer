#include <llvm-c/Analysis.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include <llvm-c/ExecutionEngine.h>
#include "../internal.h"
#include "../llvm.h"

typedef struct HLLVMParser_ {
  LLVMModuleRef mod;
  LLVMValueRef func;
  LLVMExecutionEngineRef engine;
  LLVMBuilderRef builder;
} HLLVMParser;

HParseResult* make_result(HArena *arena, HParsedToken *tok) {
  HParseResult *ret = h_arena_malloc(arena, sizeof(HParseResult));
  ret->ast = tok;
  ret->arena = arena;
  ret->bit_length = 0; // This way it gets overridden in h_do_parse
  return ret;
}

void h_llvm_declare_common(LLVMModuleRef mod) {
  llvm_inputstream = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HInputStream_");
  LLVMTypeRef llvm_inputstream_struct_types[] = {
    LLVMPointerType(LLVMInt8Type(), 0),
    LLVMInt64Type(),
    LLVMInt64Type(),
    LLVMInt64Type(),
    LLVMInt8Type(),
    LLVMInt8Type(),
    LLVMInt8Type(),
    LLVMInt8Type(),
    LLVMInt8Type()
  };
  LLVMStructSetBody(llvm_inputstream, llvm_inputstream_struct_types, 9, 0);
  llvm_inputstreamptr = LLVMPointerType(llvm_inputstream, 0);
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
    llvm_inputstreamptr,
    LLVMInt32Type(),
    LLVMInt8Type()
  };
  LLVMTypeRef readbits_ret = LLVMFunctionType(LLVMInt64Type(), readbits_pt, 3, 0);
  LLVMAddFunction(mod, "h_read_bits", readbits_ret);

  LLVMTypeRef amalloc_pt[] = {
    llvm_arenaptr,
    LLVMInt32Type()
  };
  LLVMTypeRef amalloc_ret = LLVMFunctionType(LLVMPointerType(LLVMVoidType(), 0), amalloc_pt, 2, 0);
  LLVMAddFunction(mod, "h_arena_malloc", amalloc_ret);

  LLVMTypeRef makeresult_pt[] = {
    llvm_arenaptr,
    llvm_parsedtokenptr
  };
  LLVMTypeRef makeresult_ret = LLVMFunctionType(llvm_parseresultptr, makeresult_pt, 2, 0);
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
    llvm_inputstreamptr,
    llvm_arenaptr
  };
  LLVMTypeRef ret_type = LLVMFunctionType(llvm_parseresultptr, param_types, 2, 0);
  LLVMValueRef parse_func = LLVMAddFunction(mod, name, ret_type);
  // Parse function is now declared; time to define it
  LLVMBuilderRef builder = LLVMCreateBuilder();
  // Translate the contents of the children of `parser` into their LLVM instruction equivalents
  if (parser->vtable->llvm(builder, parse_func, mod, parser->env)) {
    // But first, verification
    char *error = NULL;
    LLVMVerifyModule(mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    error = NULL;
    // OK, link that sonofabitch
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMExecutionEngineRef engine = NULL;
    LLVMCreateExecutionEngineForModule(&engine, mod, &error);
    if (error) {
      fprintf(stderr, "error: %s\n", error);
      LLVMDisposeMessage(error);
      return -1;
    }
    char* dump = LLVMPrintModuleToString(mod);
    fprintf(stderr, "\n\n%s\n\n", dump);
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
  HArena *arena = h_new_arena(mm__, 0);
  LLVMGenericValueRef args[] = {
    LLVMCreateGenericValueOfPointer(input_stream),
    LLVMCreateGenericValueOfPointer(arena)
  };
  LLVMGenericValueRef res = LLVMRunFunction(llvm_parser->engine, llvm_parser->func, 2, args);
  HParseResult *ret = (HParseResult*)LLVMGenericValueToPointer(res);
  if (ret) {
    ret->arena = arena;
    if (!input_stream->overrun) {
      size_t bit_length = h_input_stream_pos(input_stream);
      if (ret->bit_length == 0) {
	ret->bit_length = bit_length;
      }
      if (ret->ast && ret->ast->bit_length != 0) {
	((HParsedToken*)(ret->ast))->bit_length = bit_length;
      }
    } else {
      ret->bit_length = 0;
    }
  } else {
    ret = NULL;
  }
  if (input_stream->overrun) {
    return NULL; // overrun is always failure.
  }
  return ret;
}

HParserBackendVTable h__llvm_backend_vtable = {
  .compile = h_llvm_compile,
  .parse = h_llvm_parse,
  .free = h_llvm_free
};
