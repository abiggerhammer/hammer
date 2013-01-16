#include "glue.h"


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

HParsedToken *h_make_token(HArena *arena, HTokenType type, void *value)
{
  HParsedToken *ret = h_arena_malloc(arena, sizeof(HParsedToken));
  ret->token_type = type;
  ret->user = value;
  return ret;
}

#define H_MAKE(TYP) \
  ((TYP ## _t *) h_arena_malloc(p->arena, sizeof(TYP ## _t)))

#define H_MAKE_TOKEN(TYP, VAL) \
  h_make_token(p->arena, TT_ ## TYP, VAL)

HParsedToken *h_carray_index(const HCountedArray *a, size_t i)
{
  assert(i < a->used);
  return a->elements[i];
}

HParsedToken *h_seq_index(const HParsedToken *p, size_t i)
{
  assert(p->token_type == TT_SEQUENCE);
  return h_carray_index(p->seq, i);
}

void *h_seq_index_user(HTokenType type, const HParsedToken *p, size_t i)
{
  HParsedToken *elem = h_seq_index(p, i);
  assert(elem->token_type == (HTokenType)type);
  return elem->user;
}
