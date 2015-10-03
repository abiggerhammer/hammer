#include "parser_internal.h"


typedef struct {
  const HParser *p;
  int64_t lower;
  int64_t upper;
} HRange;

static HParseResult* parse_int_range(void *env, HParseState *state) {
  HRange *r_env = (HRange*)env;
  HParseResult *ret = h_do_parse(r_env->p, state);
  if (!ret || !ret->ast)
    return NULL;
  switch(ret->ast->token_type) {
  case TT_SINT:
    if (r_env->lower <= ret->ast->sint && r_env->upper >= ret->ast->sint)
      return ret;
    else
      return NULL;
  case TT_UINT:
    if ((uint64_t)r_env->lower <= ret->ast->uint && (uint64_t)r_env->upper >= ret->ast->uint)
      return ret;
    else
      return NULL;
  default:
    return NULL;
  }
}

void gen_int_range(HAllocator *mm__, HCFStack *stk__, uint64_t low, uint64_t high, uint8_t bytes) {
  /* Possible FIXME: TallerThanMe */
  if (1 == bytes) {
    HCharset cs = new_charset(mm__);
    for (uint64_t i=low; i<=high; ++i) {
      charset_set(cs, i, 1);
    }
    HCFS_ADD_CHARSET(cs);
  }
  else if (1 < bytes) {
    uint8_t low_head, hi_head;
    low_head = ((low >> (8*(bytes - 1))) & 0xFF);
    hi_head = ((high >> (8*(bytes - 1))) & 0xFF);
    if (low_head != hi_head) {
      HCFS_BEGIN_CHOICE() {
	HCFS_BEGIN_SEQ() {
	  HCFS_ADD_CHAR(low_head);
	  gen_int_range(mm__, stk__, low & ((1 << (8 * (bytes - 1))) - 1), ((1 << (8*(bytes-1)))-1), bytes-1);
	} HCFS_END_SEQ();
	HCFS_BEGIN_SEQ() {
	  HCharset hd = new_charset(mm__);
	  HCharset rest = new_charset(mm__);
	  for (int i = 0; i < 256; i++) {
	    charset_set(hd, i, (i > low_head && i < hi_head));
	    charset_set(rest, i, 1);
	  }
	  HCFS_ADD_CHARSET(hd);
	  for (int i = 2; i < bytes; i++)
	    HCFS_ADD_CHARSET(rest);
	} HCFS_END_SEQ();
	HCFS_BEGIN_SEQ() {
	  HCFS_ADD_CHAR(hi_head);
	  gen_int_range(mm__, stk__, 0, high & ((1 << (8 * (bytes - 1))) - 1), bytes-1);
	} HCFS_END_SEQ();
      } HCFS_END_CHOICE();
    } else {
      // TODO: find a way to merge this with the higher-up SEQ
      HCFS_BEGIN_CHOICE() {
	HCFS_BEGIN_SEQ() {
	  HCFS_ADD_CHAR(low_head);
	  gen_int_range(mm__, stk__,
			low & ((1 << (8 * (bytes - 1))) - 1),
			high & ((1 << (8 * (bytes - 1))) - 1),
			bytes - 1);
	} HCFS_END_SEQ();
      } HCFS_END_CHOICE();
    }
  }
}

struct bits_env {
  uint8_t length;
  uint8_t signedp;
};

static void desugar_int_range(HAllocator *mm__, HCFStack *stk__, void *env) {
  HRange *r = (HRange*)env;
  struct bits_env* be = (struct bits_env*)r->p->env;
  uint8_t bytes = be->length / 8;
  gen_int_range(mm__, stk__, r->lower, r->upper, bytes);
}

bool h_svm_action_validate_int_range(HArena *arena, HSVMContext *ctx, void* env) {
  HRange *r_env = (HRange*)env;
  HParsedToken *head = ctx->stack[ctx->stack_count-1];
  switch (head-> token_type) {
  case TT_SINT: 
    return head->sint >= r_env->lower && head->sint <= r_env->upper;
  case TT_UINT: 
    return head->uint >= (uint64_t)r_env->lower && head->uint <= (uint64_t)r_env->upper;
  default:
    return false;
  }
}

static bool ir_ctrvm(HRVMProg *prog, void *env) {
  HRange *r_env = (HRange*)env;
  
  h_compile_regex(prog, r_env->p);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_validate_int_range, env));
  return false;
}

static const HParserVtable int_range_vt = {
  .parse = parse_int_range,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_int_range,
  .compile_to_rvm = ir_ctrvm,
  .higher = false,
};

HParser* h_int_range(const HParser *p, const int64_t lower, const int64_t upper) {
  return h_int_range__m(&system_allocator, p, lower, upper);
}
HParser* h_int_range__m(HAllocator* mm__, const HParser *p, const int64_t lower, const int64_t upper) {
  // p must be an integer parser, which means it's using parse_bits
  // TODO: re-add this check
  //assert_message(p->vtable == &bits_vt, "int_range requires an integer parser"); 

  // and regardless, the bounds need to fit in the parser in question
  // TODO: check this as well.

  HRange *r_env = h_new(HRange, 1);
  r_env->p = p;
  r_env->lower = lower;
  r_env->upper = upper;
  return h_new_parser(mm__, &int_range_vt, r_env);
}
