#include <stdarg.h>
#include "parser_internal.h"

typedef struct {
  size_t len;
  const HParser **p_array;
} HSequence;


static HParseResult* parse_choice(void *env, HParseState *state) {
  HSequence *s = (HSequence*)env;
  HInputStream backup = state->input_stream;
  for (size_t i=0; i<s->len; ++i) {
    if (i != 0)
      state->input_stream = backup;
    HParseResult *tmp = h_do_parse(s->p_array[i], state);
    if (NULL != tmp)
      return tmp;
  }
  // nothing succeeded, so fail
  return NULL;
}

static const HParserVtable choice_vt = {
  .parse = parse_choice,
};

const HParser* h_choice(const HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  const HParser* ret = h_choice__mv(&system_allocator, p,  ap);
  va_end(ap);
  return ret;
}

const HParser* h_choice__m(HAllocator* mm__, const HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  const HParser* ret = h_choice__mv(mm__, p,  ap);
  va_end(ap);
  return ret;
}

const HParser* h_choice__v(const HParser* p, va_list ap) {
  return h_choice__mv(&system_allocator, p, ap);
}

const HParser* h_choice__mv(HAllocator* mm__, const HParser* p, va_list ap_) {
  va_list ap;
  size_t len = 0;
  HSequence *s = h_new(HSequence, 1);

  const HParser *arg;
  va_copy(ap, ap_);
  do {
    len++;
    arg = va_arg(ap, const HParser *);
  } while (arg);
  va_end(ap);
  s->p_array = h_new(const HParser *, len);

  va_copy(ap, ap_);
  s->p_array[0] = p;
  for (size_t i = 1; i < len; i++) {
    s->p_array[i] = va_arg(ap, const HParser *);
  } while (arg);
  va_end(ap);

  s->len = len;
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &choice_vt; ret->env = (void*)s;
  return ret;
}

