#include "glue.h"
#include "../src/internal.h"  // for h_carray_*


// The action equivalent of h_ignore.
const HParsedToken *act_ignore(const HParseResult *p)
{
  return NULL;
}

// Helper to build HAction's that pick one index out of a sequence.
const HParsedToken *act_index(int i, const HParseResult *p)
{
    if(!p) return NULL;

    const HParsedToken *tok = p->ast;

    if(!tok || tok->token_type != TT_SEQUENCE)
        return NULL;

    const HCountedArray *seq = tok->seq;
    size_t n = seq->used;

    if(i<0 || (size_t)i>=n)
        return NULL;
    else
        return tok->seq->elements[i];
}

const HParsedToken *act_index0(const HParseResult *p)
{
    return act_index(0, p);
}

void h_seq_snoc(HParsedToken *xs, const HParsedToken *x)
{
  assert(xs != NULL);
  assert(xs->token_type == TT_SEQUENCE);

  h_carray_append(xs->seq, (void *)x);
}

void h_seq_append(HParsedToken *xs, const HParsedToken *ys)
{
  assert(xs != NULL);
  assert(xs->token_type == TT_SEQUENCE);
  assert(ys != NULL);
  assert(ys->token_type == TT_SEQUENCE);

  for(size_t i; i<ys->seq->used; i++)
	h_carray_append(xs->seq, ys->seq->elements[i]);
}

// Flatten nested sequences. Always returns a sequence.
// If input element is not a sequence, returns it as a singleton sequence.
const HParsedToken *h_token_flatten(HArena *arena, const HParsedToken *p)
{
  assert(p != NULL);

  HParsedToken *ret = h_make_token_seq(arena);
  switch(p->token_type) {
  case TT_SEQUENCE:
    // Flatten and append all.
    for(size_t i; i<p->seq->used; i++) {
	  h_seq_append(ret, h_token_flatten(arena, h_seq_index(p, i)));
	}
	break;
  default:
    // Make singleton sequence.
    h_seq_snoc(ret, p);
	break;
  }

  return ret;
}

// Action version of h_token_flatten.
const HParsedToken *act_flatten(const HParseResult *p) {
  return h_token_flatten(p->arena, p->ast);
}

HParsedToken *h_make_token_(HArena *arena, HTokenType type)
{
  HParsedToken *ret = h_arena_malloc(arena, sizeof(HParsedToken));
  ret->token_type = type;
  return ret;
}

HParsedToken *h_make_token_seq(HArena *arena)
{
  return h_make_token_(arena, TT_SEQUENCE);
}

HParsedToken *h_make_token(HArena *arena, HTokenType type, void *value)
{
  HParsedToken *ret = h_make_token_(arena, type);
  ret->user = value;
  return ret;
}

HParsedToken *h_carray_index(const HCountedArray *a, size_t i)
{
  assert(i < a->used);
  return a->elements[i];
}

HParsedToken *h_seq_index(const HParsedToken *p, size_t i)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return h_carray_index(p->seq, i);
}

void *h_seq_index_user(HTokenType type, const HParsedToken *p, size_t i)
{
  HParsedToken *elem = h_seq_index(p, i);
  assert(elem->token_type == (HTokenType)type);
  return elem->user;
}
