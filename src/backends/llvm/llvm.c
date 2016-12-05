#ifdef HAMMER_LLVM_BACKEND

#include <llvm-c/Analysis.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include <llvm-c/ExecutionEngine.h>
#include "../../internal.h"
#include "llvm.h"

typedef struct HLLVMParser_ {
  LLVMModuleRef mod;
  LLVMValueRef func;
  LLVMExecutionEngineRef engine;
  LLVMBuilderRef builder;
  HLLVMParserCompileContext *compile_ctxt;
} HLLVMParser;

HParseResult* make_result(HArena *arena, HParsedToken *tok) {
  HParseResult *ret = h_arena_malloc(arena, sizeof(HParseResult));
  ret->ast = tok;
  ret->arena = arena;
  ret->bit_length = 0; // This way it gets overridden in h_do_parse
  return ret;
}

void h_llvm_declare_common(HLLVMParserCompileContext *ctxt) {
#if SIZE_MAX == 0xffffffffffffffff
  ctxt->llvm_size_t = LLVMInt64Type();
#elif SIZE_MAX == 0xffffffff
  ctxt->llvm_size_t = LLVMInt32Type();
#else
#error "SIZE_MAX is not consistent with either 64 or 32-bit platform, couldn't guess LLVM type for size_t"
#endif
  ctxt->llvm_inputstream = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HInputStream_");
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
  LLVMStructSetBody(ctxt->llvm_inputstream, llvm_inputstream_struct_types, 9, 0);
  ctxt->llvm_inputstreamptr = LLVMPointerType(ctxt->llvm_inputstream, 0);
  ctxt->llvm_arena = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HArena_");
  ctxt->llvm_arenaptr = LLVMPointerType(ctxt->llvm_arena, 0);
  ctxt->llvm_parsedtoken = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HParsedToken_");
  LLVMTypeRef llvm_parsedtoken_struct_types[] = {
    LLVMInt32Type(), // actually an enum value
    LLVMInt64Type(), // actually this is a union; the largest thing in it is 64 bits
    ctxt->llvm_size_t,
    ctxt->llvm_size_t,
    LLVMInt8Type()
  };
  LLVMStructSetBody(ctxt->llvm_parsedtoken, llvm_parsedtoken_struct_types, 5, 0);
  ctxt->llvm_parsedtokenptr = LLVMPointerType(ctxt->llvm_parsedtoken, 0);
  /* The HBytes struct is one of the cases for the union in HParsedToken */
  ctxt->llvm_hbytes = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HBytes_");
  LLVMTypeRef llvm_hbytes_struct_types[] = {
    LLVMPointerType(LLVMInt8Type(), 0), /* HBytes.token */
    ctxt->llvm_size_t                   /* HBytes.len */
  };
  LLVMStructSetBody(ctxt->llvm_hbytes, llvm_hbytes_struct_types, 2, 0);
  ctxt->llvm_hbytesptr = LLVMPointerType(ctxt->llvm_hbytes, 0);
  ctxt->llvm_parseresult = LLVMStructCreateNamed(LLVMGetGlobalContext(), "struct.HParseResult_");
  LLVMTypeRef llvm_parseresult_struct_types[] = {
    ctxt->llvm_parsedtokenptr,
    LLVMInt64Type(),
    ctxt->llvm_arenaptr
  };
  LLVMStructSetBody(ctxt->llvm_parseresult, llvm_parseresult_struct_types, 3, 0);
  ctxt->llvm_parseresultptr = LLVMPointerType(ctxt->llvm_parseresult, 0);
  LLVMTypeRef readbits_pt[] = {
    ctxt->llvm_inputstreamptr,
    LLVMInt32Type(),
    LLVMInt8Type()
  };
  LLVMTypeRef readbits_ret = LLVMFunctionType(LLVMInt64Type(), readbits_pt, 3, 0);
  LLVMAddFunction(ctxt->mod, "h_read_bits", readbits_ret);

  LLVMTypeRef amalloc_pt[] = {
    ctxt->llvm_arenaptr,
    LLVMInt32Type()
  };
  LLVMTypeRef amalloc_ret = LLVMFunctionType(LLVMPointerType(LLVMVoidType(), 0), amalloc_pt, 2, 0);
  LLVMAddFunction(ctxt->mod, "h_arena_malloc", amalloc_ret);

  LLVMTypeRef makeresult_pt[] = {
    ctxt->llvm_arenaptr,
    ctxt->llvm_parsedtokenptr
  };
  LLVMTypeRef makeresult_ret = LLVMFunctionType(ctxt->llvm_parseresultptr, makeresult_pt, 2, 0);
  LLVMAddFunction(ctxt->mod, "make_result", makeresult_ret);
}

int h_llvm_compile(HAllocator* mm__, HParser* parser, const void* params) {
  HLLVMParserCompileContext *ctxt;
  // Boilerplate to set up a translation unit, aka a module.
  const char* name = params ? (const char*)params : "parse";

  /* Build a parser compilation context */
  ctxt = h_new(HLLVMParserCompileContext, 1);
  memset(ctxt, 0, sizeof(*ctxt));
  ctxt->mm__ = mm__;
  ctxt->mod = LLVMModuleCreateWithName(name);
  h_llvm_declare_common(ctxt);

  // Boilerplate to set up the parser function to add to the module. It takes an HInputStream* and
  // returns an HParseResult.
  LLVMTypeRef param_types[] = {
    ctxt->llvm_inputstreamptr,
    ctxt->llvm_arenaptr
  };
  LLVMTypeRef ret_type = LLVMFunctionType(ctxt->llvm_parseresultptr, param_types, 2, 0);
  ctxt->func = LLVMAddFunction(ctxt->mod, name, ret_type);

  // Parse function is now declared; time to define it
  ctxt->builder = LLVMCreateBuilder();
  LLVMBasicBlockRef preamble = LLVMAppendBasicBlock(ctxt->func, "preamble");
  LLVMPositionBuilderAtEnd(ctxt->builder, preamble);

  /*
   * First thing it needs to do is get its stream and arena args and stick
   * value refs in the context.
   *
   * XXX do we always need arena?  Can we make a dummy valueref the generated
   * IR refers to, and then fill in arena if we need it after we know whether
   * we need it?  Similar concerns apply to setting up storage needed for, e.g.
   * memoizing charsets.
   */
  ctxt->stream = LLVMBuildBitCast(ctxt->builder, LLVMGetFirstParam(ctxt->func),
      ctxt->llvm_inputstreamptr, "stream");
  ctxt->arena = LLVMGetLastParam(ctxt->func);

  // Translate the contents of the children of `parser` into their LLVM instruction equivalents
  if (parser->vtable->llvm(ctxt, parser->env)) {
    // But first, verification
    char *error = NULL;
    LLVMVerifyModule(ctxt->mod, LLVMAbortProcessAction, &error);
    LLVMDisposeMessage(error);
    error = NULL;
    // OK, link that sonofabitch
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMExecutionEngineRef engine = NULL;
    LLVMCreateExecutionEngineForModule(&engine, ctxt->mod, &error);
    if (error) {
      fprintf(stderr, "error: %s\n", error);
      LLVMDisposeMessage(error);
      return -1;
    }
    char* dump = LLVMPrintModuleToString(ctxt->mod);
    fprintf(stderr, "\n\n%s\n\n", dump);
    // Package up the pointers that comprise the module and stash it in the original HParser
    HLLVMParser *llvm_parser = h_new(HLLVMParser, 1);
    llvm_parser->mod = ctxt->mod;
    llvm_parser->func = ctxt->func;
    llvm_parser->engine = engine;
    llvm_parser->builder = ctxt->builder;
    llvm_parser->compile_ctxt = ctxt;
    parser->backend_data = llvm_parser;
    return 0;
  } else {
    return -1;
  }
}

void h_llvm_free(HParser *parser) {
  HAllocator *mm__;
  HLLVMParser *llvm_parser = parser->backend_data;
  LLVMModuleRef mod_out;
  char *err_out;

  mm__ = llvm_parser->compile_ctxt->mm__;
  h_free(llvm_parser->compile_ctxt);
  llvm_parser->compile_ctxt = NULL;
  mm__ = NULL;

  llvm_parser->func = NULL;
  LLVMRemoveModule(llvm_parser->engine, llvm_parser->mod, &mod_out, &err_out);
  LLVMDisposeExecutionEngine(llvm_parser->engine);
  llvm_parser->engine = NULL;

  LLVMDisposeBuilder(llvm_parser->builder);
  llvm_parser->builder = NULL;

  LLVMDisposeModule(llvm_parser->mod);
  llvm_parser->mod = NULL;
}

HParseResult *h_llvm_parse(HAllocator* mm__, const HParser* parser, HInputStream *input_stream) {
  const HLLVMParser *llvm_parser = parser->backend_data;
  HArena *arena = h_new_arena(mm__, 0);

  // LLVMRunFunction only supports certain signatures for dumb reasons; it's this hack with
  // memcpy and function pointers, or writing a shim in LLVM IR.
  //
  // LLVMGenericValueRef args[] = {
  //   LLVMCreateGenericValueOfPointer(input_stream),
  //   LLVMCreateGenericValueOfPointer(arena)
  // };
  // LLVMGenericValueRef res = LLVMRunFunction(llvm_parser->engine, llvm_parser->func, 2, args);
  // HParseResult *ret = (HParseResult*)LLVMGenericValueToPointer(res);

  void *parse_func_ptr_v;
  HParseResult * (*parse_func_ptr)(HInputStream *input_stream, HArena *arena);
  parse_func_ptr_v = LLVMGetPointerToGlobal(llvm_parser->engine, llvm_parser->func);
  memcpy(&parse_func_ptr, &parse_func_ptr_v, sizeof(parse_func_ptr));
  HParseResult *ret = parse_func_ptr(input_stream, arena);
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

#endif /* defined(HAMMER_LLVM_BACKEND) */
