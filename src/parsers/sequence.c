#include <stdarg.h>
#include "parser_internal.h"

typedef struct {
  size_t len;
  const HParser **p_array;
} HSequence;

static HParseResult* parse_sequence(void *env, HParseState *state) {
  HSequence *s = (HSequence*)env;
  HCountedArray *seq = h_carray_new_sized(state->arena, (s->len > 0) ? s->len : 4);
  for (size_t i=0; i<s->len; ++i) {
    HParseResult *tmp = h_do_parse(s->p_array[i], state);
    // if the interim parse fails, the whole thing fails
    if (NULL == tmp) {
      return NULL;
    } else {
      if (tmp->ast)
	h_carray_append(seq, (void*)tmp->ast);
    }
  }
  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_SEQUENCE; tok->seq = seq;
  return make_result(state, tok);
}

static bool sequence_isValidRegular(void *env) {
  HSequence *s = (HSequence*)env;
  for (size_t i=0; i<s->len; ++i) {
    if (!s->p_array[i]->vtable->isValidRegular(s->p_array[i]->env))
      return false;
  }
  return true;
}

static bool sequence_isValidCF(void *env) {
  HSequence *s = (HSequence*)env;
  for (size_t i=0; i<s->len; ++i) {
    if (!s->p_array[i]->vtable->isValidCF(s->p_array[i]->env))
      return false;
  }
  return true;
}

static bool sequence_ctrvm(struct HRVMProg_ *prog, void* env) {
  HSequence *s = (HSequence*)env;
  for (size_t i=0; i<s->len; ++i) {
    if (!s->p_array[i]->vtable->compile_to_rvm(prog, s->p_array[i]->env))
      return false;
  }
  return true;
}

static const HParserVtable sequence_vt = {
  .parse = parse_sequence,
  .isValidRegular = sequence_isValidRegular,
  .isValidCF = sequence_isValidCF,
  .compile_to_rvm = sequence_ctrvm,
};

const HParser* h_sequence(const HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  const HParser* ret = h_sequence__mv(&system_allocator, p,  ap);
  va_end(ap);
  return ret;
}

const HParser* h_sequence__m(HAllocator* mm__, const HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  const HParser* ret = h_sequence__mv(mm__, p,  ap);
  va_end(ap);
  return ret;
}

const HParser* h_sequence__v(const HParser* p, va_list ap) {
  return h_sequence__mv(&system_allocator, p, ap);
}

const HParser* h_sequence__mv(HAllocator* mm__, const HParser *p, va_list ap_) {
  va_list ap;
  size_t len = 0;
  const HParser *arg;
  va_copy(ap, ap_);
  do {
    len++;
    arg = va_arg(ap, const HParser *);
  } while (arg);
  va_end(ap);
  HSequence *s = h_new(HSequence, 1);
  s->p_array = h_new(const HParser *, len);

  va_copy(ap, ap_);
  s->p_array[0] = p;
  for (size_t i = 1; i < len; i++) {
    s->p_array[i] = va_arg(ap, const HParser *);
  } while (arg);
  va_end(ap);

  s->len = len;
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &sequence_vt; ret->env = (void*)s;
  return ret;
}
