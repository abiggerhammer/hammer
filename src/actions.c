// XXX to be consolidated with glue.c when merged upstream

#include <assert.h>
#include "hammer.h"
#include "internal.h"
#include "parsers/parser_internal.h"


const HParsedToken *h_act_first(const HParseResult *p) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[0];
}

const HParsedToken *h_act_second(const HParseResult *p) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[1];
}

const HParsedToken *h_act_last(const HParseResult *p) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[p->ast->seq->used-1];
}

static void act_flatten_(HCountedArray *seq, const HParsedToken *tok) {
  if(tok == NULL) {
    return;
  } else if(tok->token_type == TT_SEQUENCE) {
    size_t i;
    for(i=0; i<tok->seq->used; i++)
      act_flatten_(seq, tok->seq->elements[i]);
  } else {
    h_carray_append(seq, (HParsedToken *)tok);
  }
}

const HParsedToken *h_act_flatten(const HParseResult *p) {
  HCountedArray *seq = h_carray_new(p->arena);

  act_flatten_(seq, p->ast);

  HParsedToken *res = a_new_(p->arena, HParsedToken, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  res->index = p->ast->index;
  res->bit_offset = p->ast->bit_offset;
  return res;
}

const HParsedToken *h_act_ignore(const HParseResult *p) {
  return NULL;
}
