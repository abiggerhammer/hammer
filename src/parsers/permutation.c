#include <stdarg.h>
#include "parser_internal.h"

typedef struct {
  size_t len;
  HParser **p_array;
} HSequence;

// main recursion, used by parse_permutation below
static int parse_permutation_tail(const HSequence *s,
                                  HCountedArray *seq,
				  const size_t k, char *set,
				  HParseState *state)
{
  // shorthands
  const size_t n = s->len;
  HParser **ps = s->p_array;

  // trivial base case
  if(k >= n)
    return 1;

  HInputStream bak = state->input_stream;

  // try available parsers as first element of the permutation tail
  HParseResult *match = NULL;
  size_t i;
  for(i=0; i<n; i++) {
    if(set[i]) {
      match = h_do_parse(ps[i], state);

      // save result
      if(match)
	seq->elements[i] = (void *)match->ast;

      // treat empty optionals (TT_NONE) like failure here
      if(match && match->ast && match->ast->token_type == TT_NONE)
        match = NULL;

      if(match) {
	// remove parser from active set
	set[i] = 0;

	// parse the rest of the permutation phrase
	if(parse_permutation_tail(s, seq, k+1, set, state)) {
	  // success
	  return 1;
	} else {
	  // place parser back in active set and try the next
	  set[i] = 1;
	}
      }

      state->input_stream = bak;  // rewind input
    }
  }

  // if all available parsers were empty optionals (TT_NONE), still succeed
  for(i=0; i<n; i++) {
    if(set[i]) {
      HParsedToken *tok = seq->elements[i];
      if(!(tok && tok->token_type == TT_NONE))
	break;
    }
  }
  if(i==n)     // all were TT_NONE
    return 1;

  // permutations exhausted
  return 0;
}

static HParseResult *parse_permutation(void *env, HParseState *state)
{
  const HSequence *s = env;
  const size_t n = s->len;

  // current set of available (not yet matched) parsers 
  char *set = h_arena_malloc(state->arena, sizeof(char) * n);
  memset(set, 1, sizeof(char) * n);

  // parse result
  HCountedArray *seq = h_carray_new_sized(state->arena, n);

  if(parse_permutation_tail(s, seq, 0, set, state)) {
    // success
    // return the sequence of results
    seq->used = n;
    HParsedToken *tok = a_new(HParsedToken, 1);
    tok->token_type  = TT_SEQUENCE;
    tok->seq = seq;
    return make_result(state->arena, tok);
  } else {
    // no parse
    // XXX free seq
    return NULL;
  }
}


static const HParserVtable permutation_vt = {
  .parse = parse_permutation,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .desugar = NULL,
  .compile_to_rvm = h_not_regular,
  .higher = true,
};

HParser* h_permutation(HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  HParser* ret = h_permutation__mv(&system_allocator, p,  ap);
  va_end(ap);
  return ret;
}

HParser* h_permutation__m(HAllocator* mm__, HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  HParser* ret = h_permutation__mv(mm__, p,  ap);
  va_end(ap);
  return ret;
}

HParser* h_permutation__v(HParser* p, va_list ap) {
  return h_permutation__mv(&system_allocator, p, ap);
}

HParser* h_permutation__mv(HAllocator* mm__, HParser* p, va_list ap_) {
  va_list ap;
  size_t len = 0;
  HSequence *s = h_new(HSequence, 1);

  HParser *arg;
  va_copy(ap, ap_);
  do {
    len++;
    arg = va_arg(ap, HParser *);
  } while (arg);
  va_end(ap);
  s->p_array = h_new(HParser *, len);

  va_copy(ap, ap_);
  s->p_array[0] = p;
  for (size_t i = 1; i < len; i++) {
    s->p_array[i] = va_arg(ap, HParser *);
  } while (arg);
  va_end(ap);

  s->len = len;
  return h_new_parser(mm__, &permutation_vt, s);
}

HParser* h_permutation__a(void *args[]) {
  return h_permutation__ma(&system_allocator, args);
}

HParser* h_permutation__ma(HAllocator* mm__, void *args[]) {
  size_t len = -1; // because do...while
  const HParser *arg;

  do {
    arg=((HParser **)args)[++len];
  } while(arg);

  HSequence *s = h_new(HSequence, 1);
  s->p_array = h_new(HParser *, len);

  for (size_t i = 0; i < len; i++) {
    s->p_array[i] = ((HParser **)args)[i];
  }

  s->len = len;
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &permutation_vt; 
  ret->env = (void*)s;
  ret->backend = PB_MIN;
  return ret;
}
