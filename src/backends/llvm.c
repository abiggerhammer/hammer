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
  LLVMModuleRef mod_out;
  char *err_out;

  llvm_parser->func = NULL;
  LLVMRemoveModule(llvm_parser->engine, llvm_parser->mod, &mod_out, &err_out);
  LLVMDisposeExecutionEngine(llvm_parser->engine);
  llvm_parser->engine = NULL;

  LLVMDisposeBuilder(llvm_parser->builder);
  llvm_parser->builder = NULL;

  LLVMDisposeModule(llvm_parser->mod);
  llvm_parser->mod = NULL;
}

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

void h_llvm_make_tt_suint(LLVMModuleRef mod, LLVMBuilderRef builder,
                          LLVMValueRef stream, LLVMValueRef arena,
                          LLVMValueRef r, LLVMValueRef *mr_out) {
  /* Set up call to h_arena_malloc() for a new HParsedToken */
  LLVMValueRef tok_size = LLVMConstInt(LLVMInt32Type(), sizeof(HParsedToken), 0);
  LLVMValueRef amalloc_args[] = { arena, tok_size };
  /* %h_arena_malloc = call void* @h_arena_malloc(%struct.HArena_.1* %1, i32 48) */
  LLVMValueRef amalloc = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "h_arena_malloc"),
      amalloc_args, 2, "h_arena_malloc");
  /* %tok = bitcast void* %h_arena_malloc to %struct.HParsedToken_.2* */
  LLVMValueRef tok = LLVMBuildBitCast(builder, amalloc, llvm_parsedtokenptr, "tok");

  /*
   * tok->token_type = TT_UINT;
   *
   * %token_type = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 0
   *
   * TODO if we handle TT_SINT too, adjust here and the zero-ext below
   */
  LLVMValueRef toktype = LLVMBuildStructGEP(builder, tok, 0, "token_type");
  /* store i32 8, i32* %token_type */
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt32Type(), 8, 0), toktype);

  /*
   * tok->uint = r;
   *
   * %token_data = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 1
   */
  LLVMValueRef tokdata = LLVMBuildStructGEP(builder, tok, 1, "token_data");
  /*
   * TODO
   *
   * This is where we'll need to adjust to handle other types (sign vs. zero extend, omit extend if
   * r is 64-bit already
   */
  LLVMBuildStore(builder, LLVMBuildZExt(builder, r, LLVMInt64Type(), "r"), tokdata);
  /*
   * Store the index from the stream into the token
   */
  /* %t_index = getelementptr inbounds %struct.HParsedToken_.2, %struct.HParsedToken_.2* %3, i32 0, i32 2 */
  LLVMValueRef tokindex = LLVMBuildStructGEP(builder, tok, 2, "t_index");
  /* %s_index = getelementptr inbounds %struct.HInputStream_.0, %struct.HInputStream_.0* %0, i32 0, i32 2 */
  LLVMValueRef streamindex = LLVMBuildStructGEP(builder, stream, 2, "s_index");
  /* %4 = load i64, i64* %s_index */
  /* store i64 %4, i64* %t_index */
  LLVMBuildStore(builder, LLVMBuildLoad(builder, streamindex, ""), tokindex);
  /* Store the bit length into the token */
  LLVMValueRef tokbitlen = LLVMBuildStructGEP(builder, tok, 3, "bit_length");
  /* TODO handle multiple bit lengths */
  LLVMBuildStore(builder, LLVMConstInt(LLVMInt64Type(), 8, 0), tokbitlen);

  /*
   * Now call make_result()
   *
   * %make_result = call %struct.HParseResult_.3* @make_result(%struct.HArena_.1* %1, %struct.HParsedToken_.2* %3)
   */
  LLVMValueRef result_args[] = { arena, tok };
  LLVMValueRef mr = LLVMBuildCall(builder, LLVMGetNamedFunction(mod, "make_result"),
      result_args, 2, "make_result");

  *mr_out = mr;
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
