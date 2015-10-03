#include <assert.h>
#include "parser_internal.h"

// TODO: split this up.
typedef struct {
  const HParser *p, *sep;
  size_t count;
  bool min_p;
} HRepeat;

static HParseResult *parse_many(void* env, HParseState *state) {
  HRepeat *env_ = (HRepeat*) env;
  HCountedArray *seq = h_carray_new_sized(state->arena, (env_->count > 0 ? env_->count : 4));
  size_t count = 0;
  HInputStream bak;
  while (env_->min_p || env_->count > count) {
    bak = state->input_stream;
    if (count > 0 && env_->sep != NULL) {
      HParseResult *sep = h_do_parse(env_->sep, state);
      if (!sep)
	goto err0;
    }
    HParseResult *elem = h_do_parse(env_->p, state);
    if (!elem)
      goto err0;
    if (elem->ast)
      h_carray_append(seq, (void*)elem->ast);
    count++;
  }
  if (count < env_->count)
    goto err;
 succ:
  ; // necessary for the label to be here...
  HParsedToken *res = a_new(HParsedToken, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  return make_result(state->arena, res);
 err0:
  if (count >= env_->count) {
    state->input_stream = bak;
    goto succ;
  }
 err:
  state->input_stream = bak;
  return NULL;
}

static bool many_isValidRegular(void *env) {
  HRepeat *repeat = (HRepeat*)env;
  return (repeat->p->vtable->isValidRegular(repeat->p->env) &&
	  (repeat->sep == NULL ||
	   repeat->sep->vtable->isValidRegular(repeat->sep->env)));
}

static bool many_isValidCF(void *env) {
  HRepeat *repeat = (HRepeat*)env;
  return (repeat->p->vtable->isValidCF(repeat->p->env) &&
	  (repeat->sep == NULL ||
	   repeat->sep->vtable->isValidCF(repeat->sep->env)));
}

// turn (_ x (_ y (_ z ()))) into (x y z) where '_' are optional
static HParsedToken *reshape_many(const HParseResult *p, void *user)
{
  HCountedArray *seq = h_carray_new(p->arena);

  const HParsedToken *tok = p->ast;
  while(tok) {
    assert(tok->token_type == TT_SEQUENCE);
    if(tok->seq->used > 0) {
      size_t n = tok->seq->used;
      assert(n <= 3);
      h_carray_append(seq, tok->seq->elements[n-2]);
      tok = tok->seq->elements[n-1];
    } else {
      tok = NULL;
    }
  }

  HParsedToken *res = a_new_(p->arena, HParsedToken, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  res->index = p->ast->index;
  res->bit_offset = p->ast->bit_offset;
  return res;
}

static void desugar_many(HAllocator *mm__, HCFStack *stk__, void *env) {
  // TODO: refactor this.
  HRepeat *repeat = (HRepeat*)env;
  if (!repeat->min_p) {
    assert(!"Unreachable");
    HCFS_BEGIN_CHOICE() {
      HCFS_BEGIN_SEQ() {
	for (size_t i = 0; i < repeat->count; i++) {
	  if (i != 0 && repeat->sep != NULL)
	    HCFS_DESUGAR(repeat->sep); // Should be ignored.
	  HCFS_DESUGAR(repeat->p);
	}
      } HCFS_END_SEQ();
    } HCFS_END_CHOICE();
    return;
  }
  if(repeat->count > 1) {
    assert_message(0, "'h_repeat_n' is not context-free, can't be desugared");
    return;
  }

  /* many(A) =>
         Ma  -> A Mar
             -> \epsilon (but not if many1/sepBy1 is used) 
         Mar -> Sep A Mar
             -> \epsilon
  */

  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      HCFS_DESUGAR(repeat->p);
      HCFS_BEGIN_CHOICE() { // Mar
	HCFS_BEGIN_SEQ() {
	  if (repeat->sep != NULL) {
	    HCFS_DESUGAR(repeat->sep);
	  }
	  //stk__->last_completed->reshape = h_act_ignore; // BUG: This modifies a memoized entry.
	  HCFS_DESUGAR(repeat->p);
	  HCFS_APPEND(HCFS_THIS_CHOICE);
	} HCFS_END_SEQ();
	HCFS_BEGIN_SEQ() {
	} HCFS_END_SEQ();
      } HCFS_END_CHOICE(); // Mar
    }
    if (repeat->count == 0) {
      HCFS_BEGIN_SEQ() {
	//HCFS_DESUGAR(h_ignore__m(mm__, h_epsilon_p()));
      } HCFS_END_SEQ();
    }
    HCFS_THIS_CHOICE->reshape = reshape_many;
  } HCFS_END_CHOICE();
}

static bool many_ctrvm(HRVMProg *prog, void *env) {
  HRepeat *repeat = (HRepeat*)env;
  uint16_t clear_to_mark = h_rvm_create_action(prog, h_svm_action_clear_to_mark, NULL);
  // TODO: implement min & max properly. Right now, it's always
  // max==inf, min={0,1}

  // Structure:
  // Min == 0:
  //        FORK end // if Min == 0
  //        GOTO mid
  //   nxt: <SEP>
  //   mid: <ELEM>
  //        FORK nxt
  //   end:

  if (repeat->min_p) {
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
    assert(repeat->count < 2); // TODO: The other cases should be supported later.
    uint16_t end_fork = 0xFFFF; // Shut up GCC
    if (repeat->count == 0)
      end_fork = h_rvm_insert_insn(prog, RVM_FORK, 0xFFFF);
    uint16_t goto_mid = h_rvm_insert_insn(prog, RVM_GOTO, 0xFFFF);
    uint16_t nxt = h_rvm_get_ip(prog);
    if (repeat->sep != NULL) {
      h_rvm_insert_insn(prog, RVM_PUSH, 0);
      if (!h_compile_regex(prog, repeat->sep))
	return false;
      h_rvm_insert_insn(prog, RVM_ACTION, clear_to_mark);
    }
    h_rvm_patch_arg(prog, goto_mid, h_rvm_get_ip(prog));
    if (!h_compile_regex(prog, repeat->p))
      return false;
    h_rvm_insert_insn(prog, RVM_FORK, nxt);
    if (repeat->count == 0)
      h_rvm_patch_arg(prog, end_fork, h_rvm_get_ip(prog));
    
    h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_make_sequence, NULL));
    return true;
  } else {
    h_rvm_insert_insn(prog, RVM_PUSH, 0);
    for (size_t i = 0; i < repeat->count; i++) {
      if (repeat->sep != NULL && i != 0) {
	h_rvm_insert_insn(prog, RVM_PUSH, 0);
	if (!h_compile_regex(prog, repeat->sep))
	  return false;
	h_rvm_insert_insn(prog, RVM_ACTION, clear_to_mark);
      }
      if (!h_compile_regex(prog, repeat->p))
	return false;
    }
    h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_make_sequence, NULL));
    return true;
  }
}

static const HParserVtable many_vt = {
  .parse = parse_many,
  .isValidRegular = many_isValidRegular,
  .isValidCF = many_isValidCF,
  .desugar = desugar_many,
  .compile_to_rvm = many_ctrvm,
  .higher = true,
};

HParser* h_many(const HParser* p) {
  return h_many__m(&system_allocator, p);
}
HParser* h_many__m(HAllocator* mm__, const HParser* p) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = NULL;
  env->count = 0;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_many1(const HParser* p) {
  return h_many1__m(&system_allocator, p);
}
HParser* h_many1__m(HAllocator* mm__, const HParser* p) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = NULL;
  env->count = 1;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_repeat_n(const HParser* p, const size_t n) {
  return h_repeat_n__m(&system_allocator, p, n);
}
HParser* h_repeat_n__m(HAllocator* mm__, const HParser* p, const size_t n) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = NULL;
  env->count = n;
  env->min_p = false;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_sepBy(const HParser* p, const HParser* sep) {
  return h_sepBy__m(&system_allocator, p, sep);
}
HParser* h_sepBy__m(HAllocator* mm__, const HParser* p, const HParser* sep) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = sep;
  env->count = 0;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_sepBy1(const HParser* p, const HParser* sep) {
  return h_sepBy1__m(&system_allocator, p, sep);
}
HParser* h_sepBy1__m(HAllocator* mm__, const HParser* p, const HParser* sep) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = sep;
  env->count = 1;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

typedef struct {
  const HParser *length;
  const HParser *value;
} HLenVal;

static HParseResult* parse_length_value(void *env, HParseState *state) {
  HLenVal *lv = (HLenVal*)env;
  HParseResult *len = h_do_parse(lv->length, state);
  if (!len)
    return NULL;
  if (len->ast->token_type != TT_UINT)
    h_platform_errx(1, "Length parser must return an unsigned integer");
  // TODO: allocate this using public functions
  HRepeat repeat = {
    .p = lv->value,
    .sep = NULL,
    .count = len->ast->uint,
    .min_p = false
  };
  return parse_many(&repeat, state);
}

static const HParserVtable length_value_vt = {
  .parse = parse_length_value,
  .isValidRegular = h_false,
  .isValidCF = h_false,
};

HParser* h_length_value(const HParser* length, const HParser* value) {
  return h_length_value__m(&system_allocator, length, value);
}
HParser* h_length_value__m(HAllocator* mm__, const HParser* length, const HParser* value) {
  HLenVal *env = h_new(HLenVal, 1);
  env->length = length;
  env->value = value;
  return h_new_parser(mm__, &length_value_vt, env);
}
