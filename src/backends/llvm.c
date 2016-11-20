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
  if (parser->vtable->llvm(mm__, builder, parse_func, mod, parser->env)) {
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

typedef enum {
  /*
   * Accept action; this entire range is in the charset.  This action type
   * has no children and terminates handling the input character.
   */
  CHARSET_ACTION_ACCEPT,
  /*
   * Scan action; test input char against each set character in the charset.
   * This action type has no children and terminates handling the input
   * character.
   */
  CHARSET_ACTION_SCAN,
  /*
   * Bitmap action; test input char against a bitmap in the IR at fixed
   * cost.
   */
  CHARSET_ACTION_BITMAP,
  /*
   * Complement action; invert the sense of the charset.  This action type
   * has one child node, with the bounds unchanged and the portion of the
   * charset within the bounds complemented.
   */
  CHARSET_ACTION_COMPLEMENT,
  /*
   * Split action; check whether the input char is above or below a split
   * point, and branch into one of two children depending.
   */
  CHARSET_ACTION_SPLIT
} llvm_charset_exec_plan_action_t;

typedef struct llvm_charset_exec_plan_s llvm_charset_exec_plan_t;
struct llvm_charset_exec_plan_s {
  /*
   * The charset at this node, with transforms such as range restriction
   * or complementation applied.
   */
  HCharset cs;
  /*
   * Char values for the range of this node, and the split point if this
   * is CHARSET_ACTION_SPLIT
   */
  uint8_t idx_start, idx_end, split_point;
  /* Action to take at this node */
  llvm_charset_exec_plan_action_t action;
  /* Estimated cost metric */
  int cost;
  /* Children, if any (zero, one or two depending on action) */
  llvm_charset_exec_plan_t *children[2];
};

/* Forward prototypes for charset llvm stuff */
static int h_llvm_build_charset_exec_plan_impl(HAllocator* mm__, HCharset cs,
    llvm_charset_exec_plan_t *parent, llvm_charset_exec_plan_t *cep,
    int allow_complement, uint8_t *split_point);
static llvm_charset_exec_plan_t * h_llvm_build_charset_exec_plan_impl_alloc(
    HAllocator* mm__, llvm_charset_exec_plan_t *parent, HCharset cs,
    uint8_t idx_start, uint8_t idx_end, int allow_complement);
static void h_llvm_free_charset_exec_plan(HAllocator* mm__,
                                          llvm_charset_exec_plan_t *cep);

/*
 * Check if this charset is eligible for CHARSET_ACTION_ACCEPT on a range
 */

static int h_llvm_charset_eligible_for_accept(HCharset cs, uint8_t idx_start, uint8_t idx_end) {
  int eligible = 1, i;

  for (i = idx_start; i <= idx_end; ++i) {
    if (!(charset_isset(cs, (uint8_t)i))) {
      eligible = 0;
      break;
    }
  }

  return eligible;
}

/*
 * Estimate cost of CHARSET_ACTION_SCAN for this charset (~proportional to number of set chars, min 1)
 */

static int h_llvm_charset_estimate_scan_cost(HCharset cs, uint8_t idx_start, uint8_t idx_end) {
  int i, cost;

  cost = 1;
  for (i = idx_start; i <= idx_end; ++i) {
    if (charset_isset(cs, (uint8_t)i)) ++cost;
  }

  return cost;
}

/*
 * Given a skeletal CHARSET_ACTION_SPLIT node from h_llvm_build_charset_exec_plan_impl(),
 * binary search for the best split point we can find and return the cost metric.
 * Unfortunately the search space is quite large, so we're going to use some silly
 * heuristics here such as looking for the longest run of present or absent chars at
 * one end of a charset, and proposing it as a split, or just trying the midpoint.
 * It may be possible to do better.
 */

static int h_llvm_find_best_split(HAllocator* mm__, llvm_charset_exec_plan_t *split) {
  int rv, best_end_run, isset, i, contiguous;
  uint8_t best_end_run_split, midpoint;
  llvm_charset_exec_plan_t *best_left, *best_right, *left, *right;
  int best_cost, cost;

  /* Sanity-check: we should be a split with a range at least two indices long */
  if (!split || split->action != CHARSET_ACTION_SPLIT) return -1;
  if (split->idx_end <= split->idx_start) return -1;

  /* Find the longest end run; split a run of length 1 at the left end as a
   * fallback, since there's always a run of length 1 at each end. */
  best_end_run = 1;
  best_end_run_split = split->idx_start;
  contiguous = 0;
  /* Try the low end */
  i = 0;
  while (i <= split->idx_end - split->idx_start &&
         (charset_isset(split->cs, split->idx_start + i) ==
          charset_isset(split->cs, split->idx_start))) ++i;
  if (i <= split->idx_end - split->idx_start) {
    /* This run has length i */
    if (i > best_end_run) {
      best_end_run = i;
      /*
       * -1 since split points are last index of left child, and i
       * is first index that wasn't in the run
       */
      best_end_run_split = split->idx_start + i - 1;
    }

    /* Now the same thing from the high end */
    i = 0;
    while (i <= split->idx_end - split->idx_start &&
           (charset_isset(split->cs, split->idx_end - i) ==
            charset_isset(split->cs, split->idx_end))) ++i;
    if (i <= split->idx_end - split->idx_start && i > best_end_run) {
      best_end_run = i;
      best_end_run_split = split->idx_end - i;
    }
  } else {
    /* Wow, contiguous - any split will turn out well - just use the midpoint */
    contiguous = 1;
  }

  /* Initialize, start trying things */
  best_left = best_right = left = right = NULL;
  rv = -1;

  /* Try a midpoint split */
  midpoint = split->idx_start + (split->idx_end - split->idx_start) / 2;
  left = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
      split->idx_start, midpoint, 1);
  right = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
      midpoint + 1, split->idx_end, 1);
  if (left && right) {
    /* Cost of the split == 1 + max(left->cost, right->cost) */
    cost = left->cost;
    if (right->cost > cost) cost = right->cost;
    ++cost;
    /* We haven't tried the end-run one yet, so always accept this */
    best_left = left;
    best_right = right;
    best_cost = cost;
    left = right = NULL;
  } else goto err;

  /*
   * Try an end-run split; if we decided we had a contiguous run earlier,
   * all are equally good, so don't bother and just use the midpoint
   */

  if (!contiguous) {
    /*
     * Sanity-check the indices; error out if the scanner gave us
     * something silly
     */
    if (best_end_run_split < split->idx_start ||
        best_end_run_split >= split->idx_end) goto err;
    left = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
        split->idx_start, best_end_run_split, 1);
    right = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
       best_end_run_split + 1, split->idx_end, 1);
    if (left && right) {
      /* Cost of the split == 1 + max(left->cost, right->cost) */
      cost = left->cost;
      if (right->cost > cost) cost = right->cost;
      ++cost;
      /* Check if against what we already have */
      if (cost < best_cost) {
        if (best_left) h_llvm_free_charset_exec_plan(mm__, best_left);
        if (best_right) h_llvm_free_charset_exec_plan(mm__, best_right);
        best_left = left;
        best_right = right;
        best_cost = cost;
        left = right = NULL;
      }
    } else goto err;
  }

  /* Set up the split node with our best results */
  split->cost = best_cost;
  split->children[0] = best_left;
  split->children[1] = best_right;
  split->split_point = best_left->idx_end;
  best_left = best_right = NULL;
  rv = split->cost;

 err:
  /* Error/cleanup case */
  if (left) h_llvm_free_charset_exec_plan(mm__, left);
  if (right) h_llvm_free_charset_exec_plan(mm__, right);
  if (best_left) h_llvm_free_charset_exec_plan(mm__, best_left);
  if (best_right) h_llvm_free_charset_exec_plan(mm__, best_right);

  return rv;
}

/*
 * Setup call to h_llvm_build_charset_exec_plan_impl(), while allocating a new
 * llvm_charset_exec_plan_t.
 */
static llvm_charset_exec_plan_t * h_llvm_build_charset_exec_plan_impl_alloc(
    HAllocator* mm__, llvm_charset_exec_plan_t *parent, HCharset cs,
    uint8_t idx_start, uint8_t idx_end, int allow_complement) {
  int cost;
  llvm_charset_exec_plan_t *cep;

  if (!mm__) return NULL;
  if (!cs) return NULL;
  if (idx_start > idx_end) return NULL;

  cep = h_new(llvm_charset_exec_plan_t, 1);
  memset(cep, 0, sizeof(*cep));
  cep->cs = copy_charset(mm__, cs);
  cep->idx_start = idx_start;
  cep->idx_end = idx_end;
  charset_restrict_to_range(cep->cs, idx_start, idx_end);
  cost = h_llvm_build_charset_exec_plan_impl(mm__, cep->cs, parent, cep,
      allow_complement, NULL);
  if (cost >= 0) cep->cost = cost;
  else {
    h_llvm_free_charset_exec_plan(mm__, cep);
    cep = NULL;
  }

  return cep;
}

/*
 * Given a charset, optionally its parent containing range restrictions, and
 * an allow_complement parameter, search for the best exec plan and write it
 * to another (skeletal) charset which will receive an action and range.  If
 * the action is CHARSET_ACTION_SPLIT, also output a split point.  Return a
 * cost estimate.
 */

static int h_llvm_build_charset_exec_plan_impl(HAllocator* mm__, HCharset cs,
    llvm_charset_exec_plan_t *parent, llvm_charset_exec_plan_t *cep,
    int allow_complement, uint8_t *split_point) {
  int eligible_for_accept, best_cost;
  int estimated_complement_cost, estimated_scan_cost, estimated_split_cost;
  int estimated_bitmap_cost;
  uint8_t idx_start, idx_end;
  HCharset complement;
  llvm_charset_exec_plan_t complement_cep, split_cep;
  llvm_charset_exec_plan_action_t chosen_action;

  /* Check args */
  if (!(mm__ && cep)) return -1;

  /*
   * The index bounds come from either the parent or maximal bounds by
   * default.  Exception is the case that we are a child of a split, in
   * which case h_llvm_find_best_split() should have set bounds in cep.
   */
  if (parent && parent->action == CHARSET_ACTION_SPLIT &&
      ((cep->idx_start == parent->idx_start &&
        cep->idx_end < parent->idx_end) ||
       (cep->idx_start > parent->idx_start &&
        cep->idx_end == parent->idx_end))) {
    idx_start = cep->idx_start;
    idx_end = cep->idx_end;
  } else if (parent) {
    idx_start = parent->idx_start;
    idx_end = parent->idx_end;
  } else {
    idx_start = 0;
    idx_end = UINT8_MAX;
  }

  eligible_for_accept = h_llvm_charset_eligible_for_accept(cs, idx_start, idx_end);
  if (eligible_for_accept) {
    /* if we can use CHARSET_ACTION_ACCEPT, always do so */
    cep->cs = copy_charset(mm__, cs);
    charset_restrict_to_range(cep->cs, idx_start, idx_end);
    cep->idx_start = idx_start;
    cep->idx_end = idx_end;
    cep->split_point = 0;
    /* Acceptance (or rejection, under a complement) is free */
    cep->cost = 0;
    cep->action = CHARSET_ACTION_ACCEPT;
    cep->children[0] = NULL;
    cep->children[1] = NULL;

    return cep->cost;
  } else {
    /*
     * Estimate cost for CHARSET_ACTION_SCAN, and for the tree below
     * CHARSET_ACTION_COMPLEMENT if we are eligible to use it.
     */
    estimated_scan_cost = h_llvm_charset_estimate_scan_cost(cs, idx_start, idx_end);
    /*
     * We can always use CHARSET_ACTION_BITMAP; this constant controls how
     * strongly we prefer it over the compare-and-branch approach.
     */
    estimated_bitmap_cost = 6;
    /* >= 0 is a flag we have a complement we may need to free later */
    estimated_complement_cost = -1;
    if (allow_complement) {
      HCharset child_cs;

      /* Complement the charset within the range */
      memset(&complement_cep, 0, sizeof(complement_cep));
      complement_cep.cs = copy_charset(mm__, cs);
      charset_restrict_to_range(complement_cep.cs, idx_start, idx_end);
      child_cs = copy_charset(mm__, complement_cep.cs);
      charset_complement(child_cs);
      charset_restrict_to_range(child_cs, idx_start, idx_end);
      complement_cep.idx_start = idx_start;
      complement_cep.idx_end = idx_end;
      complement_cep.split_point = 0;
      complement_cep.action = CHARSET_ACTION_COMPLEMENT;
      complement_cep.children[0] = h_new(llvm_charset_exec_plan_t, 1);
      memset(complement_cep.children[0], 0, sizeof(llvm_charset_exec_plan_t));
      complement_cep.children[1] = NULL;
      /*
       * Find the child; the complement has cost 0 since it just swizzles success
       * and fail output basic blocks; it's important we test for complement last
       * below then, so we break ties in favor of not stacking complements up.  We
       * set allow_complement = 0 so we never stack two complements.
       */
      complement_cep.cost = h_llvm_build_charset_exec_plan_impl(mm__, child_cs, &complement_cep,
          complement_cep.children[0], 0, NULL);
      estimated_complement_cost = complement_cep.cost;
      h_free(child_cs);
    }

    /* Set up split node */
    split_cep.cs = copy_charset(mm__, cs);
    charset_restrict_to_range(split_cep.cs, idx_start, idx_end);
    split_cep.idx_start = idx_start;
    split_cep.idx_end = idx_end;
    split_cep.split_point = 0;
    split_cep.action = CHARSET_ACTION_SPLIT;
    split_cep.cost = -1;
    split_cep.children[0] = NULL;
    split_cep.children[1] = NULL;
    estimated_split_cost = h_llvm_find_best_split(mm__, &split_cep);

    /* Pick the action type with the lowest cost */
    best_cost = -1;
    if (estimated_scan_cost >= 0 &&
        (best_cost < 0 || estimated_scan_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_SCAN;
      best_cost = estimated_scan_cost;
    }

    if (estimated_bitmap_cost >= 0 &&
        (best_cost < 0 || estimated_bitmap_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_BITMAP;
      best_cost = estimated_bitmap_cost;
    }

    if (estimated_split_cost >= 0 &&
        (best_cost < 0 || estimated_split_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_SPLIT;
      best_cost = estimated_split_cost;
    }

    if (allow_complement && estimated_complement_cost >= 0 &&
        (best_cost < 0 || estimated_complement_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_COMPLEMENT;
      best_cost = estimated_complement_cost;
    }

    /* Fill out cep based on the chosen action */
    switch (chosen_action) {
      case CHARSET_ACTION_SCAN:
        /* Set up a scan */
        cep->cs = copy_charset(mm__, cs);
        charset_restrict_to_range(cep->cs, idx_start, idx_end);
        cep->idx_start = idx_start;
        cep->idx_end = idx_end;
        cep->split_point = 0;
        cep->action = CHARSET_ACTION_SCAN;
        cep->cost = estimated_scan_cost;
        cep->children[0] = NULL;
        cep->children[1] = NULL;
        break;
      case CHARSET_ACTION_BITMAP:
        /* Set up a bitmap */
        cep->cs = copy_charset(mm__, cs);
        charset_restrict_to_range(cep->cs, idx_start, idx_end);
        cep->idx_start = idx_start;
        cep->idx_end = idx_end;
        cep->split_point = 0;
        cep->action = CHARSET_ACTION_BITMAP;
        cep->cost = estimated_bitmap_cost;
        cep->children[0] = NULL;
        cep->children[1] = NULL;
        break;
      case CHARSET_ACTION_COMPLEMENT:
        /*
         * We have a CEP filled out we can just copy over; be sure to set
         * estimated_complement_cost = -1 so we know not to free it on the
         * way out.
         */
        memcpy(cep, &complement_cep, sizeof(complement_cep));
        memset(&complement_cep, 0, sizeof(complement_cep));
        estimated_complement_cost = -1;
        break;
      case CHARSET_ACTION_SPLIT:
        /*
         * We have a CEP filled out we can just copy over; be sure to set
         * estimated_split_cost = -1 so we know not to free it on the way
         * out.
         */
        memcpy(cep, &split_cep, sizeof(split_cep));
        memset(&split_cep, 0, sizeof(split_cep));
        estimated_split_cost = -1;
        break;
      default:
        /* Not supported */
        best_cost = -1;
        memset(cep, 0, sizeof(*cep));
        break;
    }
  }

  /* Free temporary CEPs if needed */

  if (estimated_complement_cost >= 0) {
    /*
     * We have a complement_cep we ended up not using; free its child and
     * charset
     */
    h_llvm_free_charset_exec_plan(mm__, complement_cep.children[0]);
    h_free(complement_cep.cs);
    memset(&complement_cep, 0, sizeof(complement_cep));
    estimated_complement_cost = -1;
  }

  if (estimated_split_cost >= 0) {
    /*
     * We have a split_cep we ended up not using; free its children and
     * charset.
     */
    h_llvm_free_charset_exec_plan(mm__, split_cep.children[0]);
    h_llvm_free_charset_exec_plan(mm__, split_cep.children[1]);
    h_free(split_cep.cs);
    memset(&split_cep, 0, sizeof(split_cep));
    estimated_split_cost = -1;
  }

  return best_cost;
}

/*
 * Build a charset exec plan for a charset
 */

static llvm_charset_exec_plan_t * h_llvm_build_charset_exec_plan(
    HAllocator* mm__, HCharset cs) {
  llvm_charset_exec_plan_t *cep = NULL;
  int best_cost;

  cep = h_new(llvm_charset_exec_plan_t, 1);
  best_cost = h_llvm_build_charset_exec_plan_impl(mm__, cs, NULL, cep, 1, NULL);

  if (best_cost < 0) {
    /* h_llvm_build_charset_exec_plan_impl() failed */
    h_free(cep);
    cep = NULL;
  }

  return cep;
}

/*
 * Consistency-check a charset exec plan
 */

static bool h_llvm_check_charset_exec_plan(llvm_charset_exec_plan_t *cep) {
  bool consistent = false;
  uint8_t i;

  if (cep) {
    /* Check that we have a charset */
    if (!(cep->cs)) goto done;
    /* Check that the range makes sense */
    if (cep->idx_start > cep->idx_end) goto done;
    /* Check that the charset is empty outside the range */
    for (i = 0; i < cep->idx_start; ++i) {
      /* Failed check */
      if (charset_isset(cep->cs, i)) goto done;
      /* Prevent wraparound */
      if (i == UINT8_MAX) break;
    }

    if (cep->idx_end < UINT8_MAX) {
      /* We break at the end */
      for (i = cep->idx_end + 1; ; ++i) {
        /* Failed check */
        if (charset_isset(cep->cs, i)) goto done;
        /* Prevent wraparound */
        if (i == UINT8_MAX) break;
      }
    }

    /* Minimum cost estimate is 1 */
    if (cep->cost < 1) goto done;

    /* No split point unlesswe're CHARSET_ACTION_SPLIT */
    if (cep->action != CHARSET_ACTION_SPLIT && cep->split_point != 0) goto done;

    /* Action type dependent part */
    switch (cep->action) {
      case CHARSET_ACTION_ACCEPT:
      case CHARSET_ACTION_SCAN:
      case CHARSET_ACTION_BITMAP:
        /* These are always okay and have no children */
        if (cep->children[0] || cep->children[1]) goto done;
        consistent = true;
        break;
      case CHARSET_ACTION_COMPLEMENT:
        /* This has one child, which should have the same range */
        if (cep->children[1]) goto done;
        if (cep->children[0]) {
          if (cep->children[0]->idx_start == cep->idx_start &&
              cep->children[0]->idx_end == cep->idx_end) {
            /* The cost cannot be lower than the child */
            if (cep->cost < cep->children[0]->cost) goto done;
            /* Okay, we're consistent if the child node is */
            consistent = h_llvm_check_charset_exec_plan(cep->children[0]);
          }
        }
        break;
      case CHARSET_ACTION_SPLIT:
        /* This has two children, which should split the range */
        if (cep->children[0] && cep->children[1]) {
          if (cep->children[0]->idx_start == cep->idx_start &&
              cep->children[0]->idx_end + 1 == cep->children[1]->idx_start &&
              cep->children[1]->idx_end == cep->idx_end) {
            /* The split point must match the children */
            if (cep->split_point != cep->children[0]->idx_end) goto done;
            /*
             * The cost must be in the range defined by the children, + 1 for
             * the comparison at most
             */
            int child_min_cost = (cep->children[0]->cost < cep->children[1]->cost) ?
              cep->children[0]->cost : cep->children[1]->cost;
            int child_max_cost = (cep->children[0]->cost > cep->children[1]->cost) ?
              cep->children[0]->cost : cep->children[1]->cost;
            if ((cep->cost < child_min_cost) || (cep->cost > child_max_cost + 1)) goto done;
            /* Okay, we're consistent if both children are */
            consistent = h_llvm_check_charset_exec_plan(cep->children[0]) &&
                         h_llvm_check_charset_exec_plan(cep->children[1]);
          }
        }
        break;
      default:
        break;
    }
  }

 done:
  return consistent;
}

/*
 * Free a charset exec plan using the supplied allocator
 */

static void h_llvm_free_charset_exec_plan(HAllocator* mm__,
                                          llvm_charset_exec_plan_t *cep) {
  int n_children, i;

  if (cep) {
    n_children = 0;
    switch (cep->action) {
      case CHARSET_ACTION_COMPLEMENT:
        n_children = 1;
        break;
      case CHARSET_ACTION_SPLIT:
        n_children = 2;
        break;
      default:
        break;
    }

    for (i = 0; i < n_children; ++i) {
      h_llvm_free_charset_exec_plan(mm__, cep->children[i]);
    }
    h_free(cep->cs);
    h_free(cep);
  }
}

/*
 * Pretty-print a charset exec plan to stdout
 */

static void h_llvm_pretty_print_charset_exec_plan_impl(HAllocator *mm__, llvm_charset_exec_plan_t *cep,
                                                       const char *pfx_on_action_line, const char *pfx,
                                                       int depth) {
  const char *action_string = NULL, *pfx_incr = NULL;
  const char *pfx_incr_child_action = NULL, *pfx_incr_last_child = NULL;
  char *next_pfx = NULL, *next_pfx_child_action_line = NULL, *next_pfx_last_child = NULL;
  int n_children = 0, i, j, next_pfx_len;
  uint8_t ch;

  if (!cep) {
    action_string = "NULL";
  } else {
    switch (cep->action) {
      case CHARSET_ACTION_ACCEPT:
        action_string = "CHARSET_ACTION_ACCEPT";
        break;
      case CHARSET_ACTION_SCAN:
        action_string = "CHARSET_ACTION_SCAN";
        break;
      case CHARSET_ACTION_BITMAP:
        action_string = "CHARSET_ACTION_BITMAP";
        break;
      case CHARSET_ACTION_COMPLEMENT:
        action_string = "CHARSET_ACTION_COMPLEMENT";
        n_children = 1;
        break;
      case CHARSET_ACTION_SPLIT:
        action_string = "CHARSET_ACTION_SPLIT";
        n_children = 2;
        break;
      default:
        action_string = "UNKNOWN";
        break;
    }
  }

  if (n_children > 0) {
    pfx_incr = " | ";
  } else {
    pfx_incr = "   ";
  }


  if (depth > 0 || strlen(pfx_on_action_line) > 0) {
    printf("%s-%s\n", pfx_on_action_line, action_string);
    pfx_incr = (n_children > 0) ? " | " : "   ";
    pfx_incr_child_action = " +-";
    pfx_incr_last_child = "   ";
  } else {
    printf("%s\n", action_string);
    pfx_incr = (n_children > 0) ? "| " : "  ";
    pfx_incr_child_action = "+-";
    pfx_incr_last_child = "  ";
  }

  /*
   * Now do the charset, 8 lines of 32 bits with spaces in between to
   * fit [] range markers and | split point marker.
   */
  int open = 0, close = 0, split = 0;
  for (ch = 0, i = 0; i < 8; ++i) {
    /* Special case: [ should go before first char on line */
    if (ch == cep->idx_start) {
      printf("%s%s [", pfx, pfx_incr);
    } else {
      printf("%s%s  ", pfx, pfx_incr);
    }
    for (j = 0; j < 32; ++j, ++ch) {
      open = close = split = 0;
      /* Figure out markers, avoid wraparound */
      if (cep->idx_start != 0 && ch + 1 == cep->idx_start && j > 0) {
        /* There should be a [ right after this char */
        open = 1;
      } else if (ch == cep->idx_end) {
        /* There should be a ] right after this char */
        close = 1;
      } else if (ch == cep->split_point &&
                 cep->action == CHARSET_ACTION_SPLIT) {
        /* There should be a | right after this char */
        split = 1;
      }

      if (charset_isset(cep->cs, ch)) printf("X");
      else printf(".");

      if (open) printf("[");
      else if (close) printf("]");
      else if (split) printf("|");
      else printf(" ");
    }
    printf("\n");
  }

  if (cep->action == CHARSET_ACTION_SPLIT) {
    printf("%s%s idx_start = %u, split_point = %u, idx_end = %u\n",
           pfx, pfx_incr, cep->idx_start, cep->split_point, cep->idx_end);
  } else {
    printf("%s%s idx_start = %u, idx_end = %u\n",
           pfx, pfx_incr, cep->idx_start, cep->idx_end);
  }

  printf("%s%s cost = %d\n", pfx, pfx_incr, cep->cost);

  if (n_children > 0) {
    if (n_children > 1) {
      next_pfx_len = strlen(pfx) + strlen(pfx_incr) + 1;
      next_pfx = h_new(char, next_pfx_len);
      snprintf(next_pfx, next_pfx_len, "%s%s", pfx, pfx_incr);
    } else {
      /* Won't be needed */
      next_pfx = NULL;
    }
    next_pfx_len = strlen(pfx) + strlen(pfx_incr_child_action) + 1;
    next_pfx_child_action_line = h_new(char, next_pfx_len);
    snprintf(next_pfx_child_action_line, next_pfx_len,
             "%s%s", pfx, pfx_incr_child_action);
    next_pfx_len = strlen(pfx) + strlen(pfx_incr_last_child) + 1;
    next_pfx_last_child = h_new(char, next_pfx_len);
    snprintf(next_pfx_last_child, next_pfx_len,
             "%s%s", pfx, pfx_incr_last_child);

    for (i = 0; i < n_children; ++i) {
      /* Space things out */
      printf("%s%s\n", pfx, pfx_incr);
      h_llvm_pretty_print_charset_exec_plan_impl(mm__, cep->children[i],
          next_pfx_child_action_line, (i + 1 == n_children) ? next_pfx_last_child : next_pfx,
          depth + 1);
    }

    if (next_pfx) h_free(next_pfx);
    h_free(next_pfx_last_child);
    h_free(next_pfx_child_action_line);
  }
}

static void h_llvm_pretty_print_charset_exec_plan(HAllocator *mm__, llvm_charset_exec_plan_t *cep) {
  /* Start at depth 0, and always emit an initial newline */
  printf("\n");
  h_llvm_pretty_print_charset_exec_plan_impl(mm__, cep, "", "", 0);
}

/*
 * Construct LLVM IR to decide if a runtime value is a member of a compile-time
 * character set, and branch depending on the result.
 *
 * Parameters:
 *  - mod [in]: an LLVMModuleRef
 *  - func [in]: an LLVMValueRef to the function to add the new basic blocks
 *  - builder [in]: an LLVMBuilderRef, positioned appropriately
 *  - r [in]: an LLVMValueRef to the value to test
 *  - cs [in]: the HCharset to test membership in
 *  - yes [in]: the basic block to branch to if r is in cs
 *  - no [in]: the basic block to branch to if r is not in cs
 */

void h_llvm_make_charset_membership_test(HAllocator* mm__,
                                         LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                         LLVMValueRef r, HCharset cs,
                                         LLVMBasicBlockRef yes, LLVMBasicBlockRef no) {
  /*
   * A charset is a 256-element bit array, 32 bytes long in total.  Ours is
   * static at compile time, so we can try to construct minimal LLVM IR for
   * this particular charset.  In particular, we should handle cases like
   * only one or two bits being set, or a long consecutive range, efficiently.
   *
   * In LLVM IR, we can test propositions like r == x, r <= x, r >= x and their
   * negations efficiently, so the challenge here is to turn a character map
   * into a minimal set of such propositions.
   *
   * TODO: actually do this; right now for the sake of a first pass we're just
   * testing r == x for every x in cs.
   */

  /* Try building a charset exec plan */
  llvm_charset_exec_plan_t *cep = h_llvm_build_charset_exec_plan(mm__, cs);
  if (cep) {
    /* For now just check it and free it */
    bool ok = h_llvm_check_charset_exec_plan(cep);
    if (ok) fprintf(stderr, "cep %p passes consistency check\n", (void *)cep);
    else fprintf(stderr, "cep %p fails consistency check\n", (void *)cep);
    h_llvm_pretty_print_charset_exec_plan(mm__, cep);
    h_llvm_free_charset_exec_plan(mm__, cep);
    cep = NULL;
  } else {
    fprintf(stderr, "got null from h_llvm_build_charset_exec_plan()\n");
  }

  for (int i = 0; i < 256; ++i) {
    if (charset_isset(cs, i)) {
      char bbname[16];
      uint8_t c = (uint8_t)i;
      snprintf(bbname, 16, "cs_memb_%02x", c);
      LLVMValueRef icmp = LLVMBuildICmp(builder, LLVMIntEQ,
          LLVMConstInt(LLVMInt8Type(), c, 0), r, "c == r");
      LLVMBasicBlockRef bb = LLVMAppendBasicBlock(func, bbname);
      LLVMBuildCondBr(builder, icmp, yes, bb);
      LLVMPositionBuilderAtEnd(builder, bb);
    }
  }

  LLVMBuildBr(builder, no);
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
