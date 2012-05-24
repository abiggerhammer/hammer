/* Parser combinators for binary formats.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "hammer.h"
#include "internal.h"
#include "allocator.h"
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <error.h>

#define a_new_(arena, typ, count) ((typ*)arena_malloc((arena), sizeof(typ)*(count)))
#define a_new(typ, count) a_new_(state->arena, typ, count)
// we can create a_new0 if necessary. It would allocate some memory and immediately zero it out.

guint djbhash(const uint8_t *buf, size_t len) {
  guint hash = 5381;
  while (len--) {
    hash = hash * 33 + *buf++;
  }
  return hash;
}

parser_cache_value_t* recall(parser_cache_key_t *k, parse_state_t *state) {
  parser_cache_value_t *cached = g_hash_table_lookup(state->cache, k);
  head_t *head = g_hash_table_lookup(state->recursion_heads, k);
  if (!head) { // No heads found
    return cached;
  } else { // Some heads found
    if (!cached && head->head_parser != k->parser && !g_slist_find(head->involved_set, k->parser)) {
      // Nothing in the cache, and the key parser is not involved
      parse_result_t *tmp = g_new(parse_result_t, 1);
      tmp->ast = NULL; tmp->arena = state->arena;
      parser_cache_value_t *ret = g_new(parser_cache_value_t, 1);
      ret->value_type = PC_RIGHT; ret->right = tmp;
      return ret;
    }
    if (g_slist_find(head->eval_set, k->parser)) {
      // Something is in the cache, and the key parser is in the eval set. Remove the key parser from the eval set of the head. 
      head->eval_set = g_slist_remove_all(head->eval_set, k->parser);
      parse_result_t *tmp_res = k->parser->fn(k->parser->env, state);
      if (tmp_res)
	tmp_res->arena = state->arena;
      // we know that cached has an entry here, modify it
      cached->value_type = PC_RIGHT;
      cached->right = tmp_res;
    }
    return cached;
  }
}

/* Setting up the left recursion. We have the LR for the rule head;
 * we modify the involved_sets of all LRs in the stack, until we 
 * see the current parser again.
 */

void setupLR(const parser_t *p, GQueue *stack, LR_t *rec_detect) {
  if (!rec_detect->head) {
    head_t *some = g_new(head_t, 1);
    some->head_parser = p; some->involved_set = NULL; some->eval_set = NULL;
    rec_detect->head = some;
  }
  size_t i = 0;
  LR_t *lr = g_queue_peek_nth(stack, i);
  while (lr && lr->rule != p) {
    lr->head = rec_detect->head;
    lr->head->involved_set = g_slist_prepend(lr->head->involved_set, (gpointer)lr->rule);
  }
}

/* If recall() returns NULL, we need to store a dummy failure in the cache and compute the
 * future parse. 
 */

parse_result_t* grow(parser_cache_key_t *k, parse_state_t *state, head_t *head) {
  // Store the head into the recursion_heads
  g_hash_table_replace(state->recursion_heads, k, head);
  parser_cache_value_t *old_cached = g_hash_table_lookup(state->cache, k);
  if (!old_cached || PC_LEFT == old_cached->value_type)
    errx(1, "impossible match");
  parse_result_t *old_res = old_cached->right;
  
  // reset the eval_set of the head of the recursion at each beginning of growth
  head->eval_set = head->involved_set;
  parse_result_t *tmp_res;
  if (k->parser) {
    tmp_res = k->parser->fn(k->parser->env, state);
    if (tmp_res)
      tmp_res->arena = state->arena;
  } else
    tmp_res = NULL;
  if (tmp_res) {
    if ((old_res->ast->index < tmp_res->ast->index) || 
	(old_res->ast->index == tmp_res->ast->index && old_res->ast->bit_offset < tmp_res->ast->bit_offset)) {
      parser_cache_value_t *v = g_new(parser_cache_value_t, 1);
      v->value_type = PC_RIGHT; v->right = tmp_res;
      g_hash_table_replace(state->cache, k, v);
      return grow(k, state, head);
    } else {
      // we're done with growing, we can remove data from the recursion head
      g_hash_table_remove(state->recursion_heads, k);
      parser_cache_value_t *cached = g_hash_table_lookup(state->cache, k);
      if (cached && PC_RIGHT == cached->value_type) {
	return cached->right;
      } else {
	errx(1, "impossible match");
      }
    }
  } else {
    g_hash_table_remove(state->recursion_heads, k);
    return old_res;
  }
}

parse_result_t* lr_answer(parser_cache_key_t *k, parse_state_t *state, LR_t *growable) {
  if (growable->head) {
    if (growable->head->head_parser != k->parser) {
      // not the head rule, so not growing
      return growable->seed;
    }
    else {
      // update cache
      parser_cache_value_t *v = g_new(parser_cache_value_t, 1);
      v->value_type = PC_RIGHT; v->right = growable->seed;
      g_hash_table_replace(state->cache, k, v);
      if (!growable->seed)
	return NULL;
      else
	return grow(k, state, growable->head);
    }
  } else {
    errx(1, "lrAnswer with no head");
  }
}

/* Warth's recursion. Hi Alessandro! */
parse_result_t* do_parse(const parser_t* parser, parse_state_t *state) {
  parser_cache_key_t *key = a_new(parser_cache_key_t, 1);
  key->input_pos = state->input_stream; key->parser = parser;
  parser_cache_value_t *m = recall(key, state);
  // check to see if there is already a result for this object...
  if (!m) {
    // It doesn't exist, so create a dummy result to cache
    LR_t *base = a_new(LR_t, 1);
    base->seed = NULL; base->rule = parser; base->head = NULL;
    g_queue_push_head(state->lr_stack, base);
    // cache it
    parser_cache_value_t *dummy = a_new(parser_cache_value_t, 1);
    dummy->value_type = PC_LEFT; dummy->left = base;
    g_hash_table_replace(state->cache, key, dummy);
    // parse the input
    parse_result_t *tmp_res;
    if (parser) {
      tmp_res = parser->fn(parser->env, state);
      if (tmp_res)
	tmp_res->arena = state->arena;
    } else
      tmp_res = NULL;
    if (state->input_stream.overrun)
      return NULL; // overrun is always failure.
#ifdef CONSISTENCY_CHECK
    if (!tmp_res) {
      state->input_stream = INVALID;
      state->input_stream.input = key->input_pos.input;
    }
#endif
    // the base variable has passed equality tests with the cache
    g_queue_pop_head(state->lr_stack);
    // setupLR, used below, mutates the LR to have a head if appropriate, so we check to see if we have one
    if (NULL == base->head) {
      parser_cache_value_t *right = a_new(parser_cache_value_t, 1);
      right->value_type = PC_RIGHT; right->right = tmp_res;
      g_hash_table_replace(state->cache, key, right);
      return tmp_res;
    } else {
      base->seed = tmp_res;
      parse_result_t *res = lr_answer(key, state, base);
      return res;
    }
  } else {
    // it exists!
    if (PC_LEFT == m->value_type) {
      setupLR(parser, state->lr_stack, m->left);
      return m->left->seed; // BUG: this might not be correct
    } else {
      return m->right;
    }
  }
}

/* Helper function, since these lines appear in every parser */
parse_result_t* make_result(parse_state_t *state, parsed_token_t *tok) {
  parse_result_t *ret = a_new(parse_result_t, 1);
  ret->ast = tok;
  ret->arena = state->arena;
  return ret;
}

typedef struct {
  uint8_t *str;
  uint8_t len;
} token_t;

static parse_result_t* parse_unimplemented(void* env, parse_state_t *state) {
  (void) env;
  (void) state;
  static parsed_token_t token = {
    .token_type = TT_ERR
  };
  static parse_result_t result = {
    .ast = &token
  };
  return &result;
}

static parser_t unimplemented = {
  .fn = parse_unimplemented,
  .env = NULL
};

struct bits_env {
  uint8_t length;
  uint8_t signedp;
};

static parse_result_t* parse_bits(void* env, parse_state_t *state) {
  struct bits_env *env_ = env;
  parsed_token_t *result = a_new(parsed_token_t, 1);
  result->token_type = (env_->signedp ? TT_SINT : TT_UINT);
  if (env_->signedp)
    result->sint = read_bits(&state->input_stream, env_->length, true);
  else
    result->uint = read_bits(&state->input_stream, env_->length, false);
  return make_result(state, result);
}

const parser_t* bits(size_t len, bool sign) {
  struct bits_env *env = g_new(struct bits_env, 1);
  env->length = len;
  env->signedp = sign;
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_bits;
  res->env = env;
  return res;
}

#define SIZED_BITS(name_pre, len, signedp) \
  const parser_t* name_pre##len () {	       \
    return bits(len, signedp);	       \
  }
SIZED_BITS(int, 8, true)
SIZED_BITS(int, 16, true)
SIZED_BITS(int, 32, true)
SIZED_BITS(int, 64, true)
SIZED_BITS(uint, 8, false)
SIZED_BITS(uint, 16, false)
SIZED_BITS(uint, 32, false)
SIZED_BITS(uint, 64, false)

static parse_result_t* parse_token(void *env, parse_state_t *state) {
  token_t *t = (token_t*)env;
  for (int i=0; i<t->len; ++i) {
    uint8_t chr = (uint8_t)read_bits(&state->input_stream, 8, false);
    if (t->str[i] != chr) {
      return NULL;
    }
  }
  parsed_token_t *tok = a_new(parsed_token_t, 1);
  tok->token_type = TT_BYTES; tok->bytes.token = t->str; tok->bytes.len = t->len;
  return make_result(state, tok);
}

const parser_t* token(const uint8_t *str, const size_t len) { 
  token_t *t = g_new(token_t, 1);
  t->str = (uint8_t*)str, t->len = len;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_token; ret->env = t;
  return (const parser_t*)ret;
}

static parse_result_t* parse_ch(void* env, parse_state_t *state) {
  uint8_t c = (uint8_t)GPOINTER_TO_UINT(env);
  uint8_t r = (uint8_t)read_bits(&state->input_stream, 8, false);
  if (c == r) {
    parsed_token_t *tok = a_new(parsed_token_t, 1);    
    tok->token_type = TT_UINT; tok->uint = r;
    return make_result(state, tok);
  } else {
    return NULL;
  }
}

const parser_t* ch(const uint8_t c) {  
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_ch; ret->env = GUINT_TO_POINTER(c);
  return (const parser_t*)ret;
}

static parse_result_t* parse_whitespace(void* env, parse_state_t *state) {
  char c;
  input_stream_t bak;
  do {
    bak = state->input_stream;
    c = read_bits(&state->input_stream, 8, false);
    if (state->input_stream.overrun)
      return NULL;
  } while (isspace(c));
  state->input_stream = bak;
  return do_parse((parser_t*)env, state);
}

const parser_t* whitespace(const parser_t* p) {
  parser_t *ret = g_new(parser_t, 1);
  ret->fn  = parse_whitespace;
  ret->env = (void*)p;
  return ret;
}

typedef struct {
  const parser_t *p;
  action_t action;
} parse_action_t;

static parse_result_t* parse_action(void *env, parse_state_t *state) {
  parse_action_t *a = (parse_action_t*)env;
  if (a->p && a->action) {
    parse_result_t *tmp = do_parse(a->p, state);
    //parsed_token_t *tok = a->action(do_parse(a->p, state));
    const parsed_token_t *tok = a->action(tmp);
    return make_result(state, (parsed_token_t*)tok);
  } else // either the parser's missing or the action's missing
    return NULL;
}

const parser_t* action(const parser_t* p, const action_t a) { 
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_action;
  parse_action_t *env = g_new(parse_action_t, 1);
  env->p = p;
  env->action = a;
  res->env = (void*)env;
  return res;
}

static parse_result_t* parse_charset(void *env, parse_state_t *state) {
  uint8_t in = read_bits(&state->input_stream, 8, false);
  charset cs = (charset)env;

  if (charset_isset(cs, in)) {
    parsed_token_t *tok = a_new(parsed_token_t, 1);
    tok->token_type = TT_UINT; tok->uint = in;
    return make_result(state, tok);    
  } else
    return NULL;
}

const parser_t* ch_range(const uint8_t lower, const uint8_t upper) {
  parser_t *ret = g_new(parser_t, 1);
  charset cs = new_charset();
  for (int i = 0; i < 256; i++)
    charset_set(cs, i, (lower <= i) && (i <= upper));
  ret->fn = parse_charset; ret->env = (void*)cs;
  return (const parser_t*)ret;
}

typedef struct {
  int64_t lower;
  int64_t upper;
} range_t;

static parse_result_t* parse_int_range(void *env, parse_state_t *state) {
  return NULL;
}

const parser_t* int_range(const parser_t *p, const int64_t lower, const int64_t upper) {
  struct bits_env *b_env = p->env;
  // p must be an integer parser, which means it's using parse_bits;
  // if it's a uint parser, it can't be uint64
  assert_message(p->fn == parse_bits, "int_range requires an integer parser"); 
  assert_message(!(b_env->signedp) ? (b_env->length < 64) : true, "int_range can't use a uint64 parser");

  range_t *r_env = g_new(range_t, 1);
  r_env->lower = lower;
  r_env->upper = upper;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_int_range;
  ret->env = (void*)r_env;
  return ret;
}

const parser_t* not_in(const uint8_t *options, int count) {
  parser_t *ret = g_new(parser_t, 1);
  charset cs = new_charset();
  for (int i = 0; i < 256; i++)
    charset_set(cs, i, 1);
  for (int i = 0; i < count; i++)
    charset_set(cs, options[i], 0);

  ret->fn = parse_charset; ret->env = (void*)cs;
  return (const parser_t*)ret;
}

static parse_result_t* parse_end(void *env, parse_state_t *state) {
  if (state->input_stream.index == state->input_stream.length) {
    parse_result_t *ret = a_new(parse_result_t, 1);
    ret->ast = NULL;
    return ret;
  } else {
    return NULL;
  }
}

const parser_t* end_p() { 
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_end; ret->env = NULL;
  return (const parser_t*)ret;
}

static parse_result_t* parse_nothing() {
  // not a mistake, this parser always fails
  return NULL;
}

const parser_t* nothing_p() { 
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_nothing; ret->env = NULL;
  return (const parser_t*)ret;
}

typedef struct {
  size_t len;
  const parser_t **p_array;
} sequence_t;

static parse_result_t* parse_sequence(void *env, parse_state_t *state) {
  sequence_t *s = (sequence_t*)env;
  counted_array_t *seq = carray_new_sized(state->arena, (s->len > 0) ? s->len : 4);
  for (size_t i=0; i<s->len; ++i) {
    parse_result_t *tmp = do_parse(s->p_array[i], state);
    // if the interim parse fails, the whole thing fails
    if (NULL == tmp) {
      return NULL;
    } else {
      if (tmp->ast)
	carray_append(seq, (void*)tmp->ast);
    }
  }
  parsed_token_t *tok = a_new(parsed_token_t, 1);
  tok->token_type = TT_SEQUENCE; tok->seq = seq;
  return make_result(state, tok);
}

const parser_t* sequence(const parser_t *p, ...) {
  va_list ap;
  size_t len = 0;
  const parser_t *arg;
  va_start(ap, p);
  do {
    len++;
    arg = va_arg(ap, const parser_t *);
  } while (arg);
  va_end(ap);
  sequence_t *s = g_new(sequence_t, 1);
  s->p_array = g_new(const parser_t *, len);

  va_start(ap, p);
  s->p_array[0] = p;
  for (size_t i = 1; i < len; i++) {
    s->p_array[i] = va_arg(ap, const parser_t *);
  } while (arg);
  va_end(ap);

  s->len = len;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_sequence; ret->env = (void*)s;
  return ret;
}

static parse_result_t* parse_choice(void *env, parse_state_t *state) {
  sequence_t *s = (sequence_t*)env;
  input_stream_t backup = state->input_stream;
  for (size_t i=0; i<s->len; ++i) {
    if (i != 0)
      state->input_stream = backup;
    parse_result_t *tmp = do_parse(s->p_array[i], state);
    if (NULL != tmp)
      return tmp;
  }
  // nothing succeeded, so fail
  return NULL;
}

const parser_t* choice(const parser_t* p, ...) {
  va_list ap;
  size_t len = 0;
  sequence_t *s = g_new(sequence_t, 1);

  const parser_t *arg;
  va_start(ap, p);
  do {
    len++;
    arg = va_arg(ap, const parser_t *);
  } while (arg);
  va_end(ap);
  s->p_array = g_new(const parser_t *, len);

  va_start(ap, p);
  s->p_array[0] = p;
  for (size_t i = 1; i < len; i++) {
    s->p_array[i] = va_arg(ap, const parser_t *);
  } while (arg);
  va_end(ap);

  s->len = len;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_choice; ret->env = (void*)s;
  return ret;
}

typedef struct {
  const parser_t *p1;
  const parser_t *p2;
} two_parsers_t;

size_t accumulate_size(parse_result_t *pr) {
  if (NULL != ((parse_result_t*)pr)->ast) {
    switch (pr->ast->token_type) {
    case TT_BYTES:
      return pr->ast->bytes.len;
    case TT_SINT:
    case TT_UINT:
      return sizeof(pr->ast->uint);
    case TT_SEQUENCE: {
      counted_array_t *arr = pr->ast->seq;
      size_t ret = 0;
      for (size_t i = 0; i < arr->used; i++)
	ret += accumulate_size(arr->elements[i]);
      return ret;
    }
    default:
      return 0;
    }
  } // no else, if the AST is null then acc doesn't change
  return 0;
}

size_t token_length(parse_result_t *pr) {
  if (NULL == pr) {
    return 0;
  } else {
    return accumulate_size(pr);
  }
}

static parse_result_t* parse_butnot(void *env, parse_state_t *state) {
  two_parsers_t *parsers = (two_parsers_t*)env;
  // cache the initial state of the input stream
  input_stream_t start_state = state->input_stream;
  parse_result_t *r1 = do_parse(parsers->p1, state);
  // if p1 failed, bail out early
  if (NULL == r1) {
    return NULL;
  } 
  // cache the state after parse #1, since we might have to back up to it
  input_stream_t after_p1_state = state->input_stream;
  state->input_stream = start_state;
  parse_result_t *r2 = do_parse(parsers->p2, state);
  // TODO(mlp): I'm pretty sure the input stream state should be the post-p1 state in all cases
  state->input_stream = after_p1_state;
  // if p2 failed, restore post-p1 state and bail out early
  if (NULL == r2) {
    return r1;
  }
  size_t r1len = token_length(r1);
  size_t r2len = token_length(r2);
  // if both match but p1's text is as long as or longer than p2's, fail
  if (r1len >= r2len) {
    return NULL;
  } else {
    return r1;
  }
}

const parser_t* butnot(const parser_t* p1, const parser_t* p2) { 
  two_parsers_t *env = g_new(two_parsers_t, 1);
  env->p1 = p1; env->p2 = p2;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_butnot; ret->env = (void*)env;
  return ret;
}

static parse_result_t* parse_difference(void *env, parse_state_t *state) {
  two_parsers_t *parsers = (two_parsers_t*)env;
  // cache the initial state of the input stream
  input_stream_t start_state = state->input_stream;
  parse_result_t *r1 = do_parse(parsers->p1, state);
  // if p1 failed, bail out early
  if (NULL == r1) {
    return NULL;
  } 
  // cache the state after parse #1, since we might have to back up to it
  input_stream_t after_p1_state = state->input_stream;
  state->input_stream = start_state;
  parse_result_t *r2 = do_parse(parsers->p2, state);
  // TODO(mlp): I'm pretty sure the input stream state should be the post-p1 state in all cases
  state->input_stream = after_p1_state;
  // if p2 failed, restore post-p1 state and bail out early
  if (NULL == r2) {
    return r1;
  }
  size_t r1len = token_length(r1);
  size_t r2len = token_length(r2);
  // if both match but p1's text is shorter than p2's, fail
  if (r1len < r2len) {
    return NULL;
  } else {
    return r1;
  }
}

const parser_t* difference(const parser_t* p1, const parser_t* p2) { 
  two_parsers_t *env = g_new(two_parsers_t, 1);
  env->p1 = p1; env->p2 = p2;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_difference; ret->env = (void*)env;
  return ret;
}

static parse_result_t* parse_xor(void *env, parse_state_t *state) {
  two_parsers_t *parsers = (two_parsers_t*)env;
  // cache the initial state of the input stream
  input_stream_t start_state = state->input_stream;
  parse_result_t *r1 = do_parse(parsers->p1, state);
  input_stream_t after_p1_state = state->input_stream;
  // reset input stream, parse again
  state->input_stream = start_state;
  parse_result_t *r2 = do_parse(parsers->p2, state);
  if (NULL == r1) {
    if (NULL != r2) {
      return r2;
    } else {
      return NULL;
    }
  } else {
    if (NULL == r2) {
      state->input_stream = after_p1_state;
      return r1;
    } else {
      return NULL;
    }
  }
}

const parser_t* xor(const parser_t* p1, const parser_t* p2) { 
  two_parsers_t *env = g_new(two_parsers_t, 1);
  env->p1 = p1; env->p2 = p2;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_xor; ret->env = (void*)env;
  return ret;
}

typedef struct {
  const parser_t *p, *sep;
  size_t count;
  bool min_p;
} repeat_t;

static parse_result_t *parse_many(void* env, parse_state_t *state) {
  repeat_t *env_ = (repeat_t*) env;
  counted_array_t *seq = carray_new_sized(state->arena, (env_->count > 0 ? env_->count : 4));
  size_t count = 0;
  input_stream_t bak;
  while (env_->min_p || env_->count > count) {
    bak = state->input_stream;
    if (count > 0) {
      parse_result_t *sep = do_parse(env_->sep, state);
      if (!sep)
	goto err0;
    }
    parse_result_t *elem = do_parse(env_->p, state);
    if (!elem)
      goto err0;
    if (elem->ast)
      carray_append(seq, (void*)elem->ast);
    count++;
  }
  if (count < env_->count)
    goto err;
 succ:
  ; // necessary for the label to be here...
  parsed_token_t *res = a_new(parsed_token_t, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  return make_result(state, res);
 err0:
  if (count >= env_->count) {
    state->input_stream = bak;
    goto succ;
  }
 err:
  state->input_stream = bak;
  return NULL;
}

const parser_t* many(const parser_t* p) {
  parser_t *res = g_new(parser_t, 1);
  repeat_t *env = g_new(repeat_t, 1);
  env->p = p;
  env->sep = epsilon_p();
  env->count = 0;
  env->min_p = true;
  res->fn = parse_many;
  res->env = env;
  return res;
}

const parser_t* many1(const parser_t* p) {
  parser_t *res = g_new(parser_t, 1);
  repeat_t *env = g_new(repeat_t, 1);
  env->p = p;
  env->sep = epsilon_p();
  env->count = 1;
  env->min_p = true;
  res->fn = parse_many;
  res->env = env;
  return res;
}

const parser_t* repeat_n(const parser_t* p, const size_t n) {
  parser_t *res = g_new(parser_t, 1);
  repeat_t *env = g_new(repeat_t, 1);
  env->p = p;
  env->sep = epsilon_p();
  env->count = n;
  env->min_p = false;
  res->fn = parse_many;
  res->env = env;
  return res;
}

static parse_result_t* parse_ignore(void* env, parse_state_t* state) {
  parse_result_t *res0 = do_parse((parser_t*)env, state);
  if (!res0)
    return NULL;
  parse_result_t *res = a_new(parse_result_t, 1);
  res->ast = NULL;
  res->arena = state->arena;
  return res;
}
const parser_t* ignore(const parser_t* p) {
  parser_t* ret = g_new(parser_t, 1);
  ret->fn = parse_ignore;
  ret->env = (void*)p;
  return ret;
}

static parse_result_t* parse_optional(void* env, parse_state_t* state) {
  input_stream_t bak = state->input_stream;
  parse_result_t *res0 = do_parse((parser_t*)env, state);
  if (res0)
    return res0;
  state->input_stream = bak;
  parsed_token_t *ast = a_new(parsed_token_t, 1);
  ast->token_type = TT_NONE;
  return make_result(state, ast);
}

const parser_t* optional(const parser_t* p) {
  assert_message(p->fn != parse_ignore, "Thou shalt ignore an option, rather than the other way 'round.");
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_optional;
  ret->env = (void*)p;
  return ret;
}

const parser_t* sepBy(const parser_t* p, const parser_t* sep) {
  parser_t *res = g_new(parser_t, 1);
  repeat_t *env = g_new(repeat_t, 1);
  env->p = p;
  env->sep = sep;
  env->count = 0;
  env->min_p = true;
  res->fn = parse_many;
  res->env = env;
  return res;
}

const parser_t* sepBy1(const parser_t* p, const parser_t* sep) {
  parser_t *res = g_new(parser_t, 1);
  repeat_t *env = g_new(repeat_t, 1);
  env->p = p;
  env->sep = sep;
  env->count = 1;
  env->min_p = true;
  res->fn = parse_many;
  res->env = env;
  return res;
}

static parse_result_t* parse_epsilon(void* env, parse_state_t* state) {
  (void)env;
  parse_result_t* res = a_new(parse_result_t, 1);
  res->ast = NULL;
  res->arena = state->arena;
  return res;
}

const parser_t* epsilon_p() {
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_epsilon;
  res->env = NULL;
  return res;
}

static parse_result_t* parse_indirect(void* env, parse_state_t* state) {
  return do_parse(env, state);
}
void bind_indirect(parser_t* indirect, parser_t* inner) {
  indirect->env = inner;
}

parser_t* indirect() {
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_indirect;
  res->env = NULL;
  return res;
}

typedef struct {
  const parser_t *p;
  predicate_t pred;
} attr_bool_t;

static parse_result_t* parse_attr_bool(void *env, parse_state_t *state) {
  attr_bool_t *a = (attr_bool_t*)env;
  parse_result_t *res = do_parse(a->p, state);
  if (res && res->ast) {
    if (a->pred(res))
      return res;
    else
      return NULL;
  } else
    return NULL;
}

const parser_t* attr_bool(const parser_t* p, predicate_t pred) { 
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_attr_bool;
  attr_bool_t *env = g_new(attr_bool_t, 1);
  env->p = p;
  env->pred = pred;
  res->env = (void*)env;
  return res;
}

typedef struct {
  const parser_t *length;
  const parser_t *value;
} lv_t;

static parse_result_t* parse_length_value(void *env, parse_state_t *state) {
  lv_t *lv = (lv_t*)env;
  parse_result_t *len = do_parse(lv->length, state);
  if (!len)
    return NULL;
  if (len->ast->token_type != TT_UINT)
    errx(1, "Length parser must return an unsigned integer");
  parser_t epsilon_local = {
    .fn = parse_epsilon,
    .env = NULL
  };
  repeat_t repeat = {
    .p = lv->value,
    .sep = &epsilon_local,
    .count = len->ast->uint,
    .min_p = false
  };
  return parse_many(&repeat, state);
}

const parser_t* length_value(const parser_t* length, const parser_t* value) {
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_length_value;
  lv_t *env = g_new(lv_t, 1);
  env->length = length;
  env->value = value;
  res->env = (void*)env;
  return res;
}

const parser_t* and(const parser_t* p) { return &unimplemented; }

static parse_result_t* parse_not(void* env, parse_state_t* state) {
  input_stream_t bak = state->input_stream;
  if (do_parse((parser_t*)env, state))
    return NULL;
  else {
    state->input_stream = bak;
    return make_result(state, NULL);
  }
}

const parser_t* not(const parser_t* p) {
  parser_t *res = g_new(parser_t, 1);
  res->fn = parse_not;
  res->env = (void*)p;
  return res;
}

static guint cache_key_hash(gconstpointer key) {
  return djbhash(key, sizeof(parser_cache_key_t));
}
static gboolean cache_key_equal(gconstpointer key1, gconstpointer key2) {
  return memcmp(key1, key2, sizeof(parser_cache_key_t)) == 0;
}


parse_result_t* parse(const parser_t* parser, const uint8_t* input, size_t length) { 
  // Set up a parse state...
  arena_t arena = new_arena(0);
  parse_state_t *parse_state = a_new_(arena, parse_state_t, 1);
  parse_state->cache = g_hash_table_new(cache_key_hash, // hash_func
					cache_key_equal);// key_equal_func
  parse_state->input_stream.input = input;
  parse_state->input_stream.index = 0;
  parse_state->input_stream.bit_offset = 8; // bit big endian
  parse_state->input_stream.overrun = 0;
  parse_state->input_stream.endianness = BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN;
  parse_state->input_stream.length = length;
  parse_state->lr_stack = g_queue_new();
  parse_state->recursion_heads = g_hash_table_new(cache_key_hash,
						  cache_key_equal);
  parse_state->arena = arena;
  parse_result_t *res = do_parse(parser, parse_state);
  g_queue_free(parse_state->lr_stack);
  g_hash_table_destroy(parse_state->recursion_heads);
  // tear down the parse state
  g_hash_table_destroy(parse_state->cache);
  if (!res)
    delete_arena(parse_state->arena);

  return res;
}

#ifdef INCLUDE_TESTS

#include "test_suite.h"
static void test_token(void) {
  const parser_t *token_ = token((const uint8_t*)"95\xa2", 3);

  g_check_parse_ok(token_, "95\xa2", 3, "<39.35.a2>");
  g_check_parse_failed(token_, "95", 2);
}

static void test_ch(void) {
  const parser_t *ch_ = ch(0xa2);

  g_check_parse_ok(ch_, "\xa2", 1, "u0xa2");
  g_check_parse_failed(ch_, "\xa3", 1);
}

static void test_ch_range(void) {
  const parser_t *range_ = ch_range('a', 'c');

  g_check_parse_ok(range_, "b", 1, "u0x62");
  g_check_parse_failed(range_, "d", 1);
}

//@MARK_START
static void test_int64(void) {
  const parser_t *int64_ = int64();

  g_check_parse_ok(int64_, "\xff\xff\xff\xfe\x00\x00\x00\x00", 8, "s-0x200000000");
  g_check_parse_failed(int64_, "\xff\xff\xff\xfe\x00\x00\x00", 7);
}

static void test_int32(void) {
  const parser_t *int32_ = int32();

  g_check_parse_ok(int32_, "\xff\xfe\x00\x00", 4, "s-0x20000");
  g_check_parse_failed(int32_, "\xff\xfe\x00", 3);
}

static void test_int16(void) {
  const parser_t *int16_ = int16();

  g_check_parse_ok(int16_, "\xfe\x00", 2, "s-0x200");
  g_check_parse_failed(int16_, "\xfe", 1);
}

static void test_int8(void) {
  const parser_t *int8_ = int8();

  g_check_parse_ok(int8_, "\x88", 1, "s-0x78");
  g_check_parse_failed(int8_, "", 0);
}

static void test_uint64(void) {
  const parser_t *uint64_ = uint64();

  g_check_parse_ok(uint64_, "\x00\x00\x00\x02\x00\x00\x00\x00", 8, "u0x200000000");
  g_check_parse_failed(uint64_, "\x00\x00\x00\x02\x00\x00\x00", 7);
}

static void test_uint32(void) {
  const parser_t *uint32_ = uint32();

  g_check_parse_ok(uint32_, "\x00\x02\x00\x00", 4, "u0x20000");
  g_check_parse_failed(uint32_, "\x00\x02\x00", 3);
}

static void test_uint16(void) {
  const parser_t *uint16_ = uint16();

  g_check_parse_ok(uint16_, "\x02\x00", 2, "u0x200");
  g_check_parse_failed(uint16_, "\x02", 1);
}

static void test_uint8(void) {
  const parser_t *uint8_ = uint8();

  g_check_parse_ok(uint8_, "\x78", 1, "u0x78");
  g_check_parse_failed(uint8_, "", 0);
}
//@MARK_END

#if 0
static void test_float64(void) {
  const parser_t *float64_ = float64();

  g_check_parse_ok(float64_, "\x3f\xf0\x00\x00\x00\x00\x00\x00", 8, 1.0);
  g_check_parse_failed(float64_, "\x3f\xf0\x00\x00\x00\x00\x00", 7);
}

static void test_float32(void) {
  const parser_t *float32_ = float32();

  g_check_parse_ok(float32_, "\x3f\x80\x00\x00", 4, 1.0);
  g_check_parse_failed(float32_, "\x3f\x80\x00");
}
#endif


static void test_whitespace(void) {
  const parser_t *whitespace_ = whitespace(ch('a'));

  g_check_parse_ok(whitespace_, "a", 1, "u0x61");
  g_check_parse_ok(whitespace_, " a", 2, "u0x61");
  g_check_parse_ok(whitespace_, "  a", 3, "u0x61");
  g_check_parse_ok(whitespace_, "\ta", 2, "u0x61");
  g_check_parse_failed(whitespace_, "_a", 2);
}

#include <ctype.h>

const parsed_token_t* upcase(parse_result_t *p) {
  switch(p->ast->token_type) {
  case TT_SEQUENCE:
    {
      parsed_token_t *ret = a_new_(p->arena, parsed_token_t, 1);
      counted_array_t *seq = carray_new_sized(p->arena, p->ast->seq->used);
      ret->token_type = TT_SEQUENCE;
      for (size_t i=0; i<p->ast->seq->used; ++i) {
	if (TT_UINT == ((parsed_token_t*)p->ast->seq->elements[i])->token_type) {
	  parsed_token_t *tmp = a_new_(p->arena, parsed_token_t, 1);
	  tmp->token_type = TT_UINT;
	  tmp->uint = toupper(((parsed_token_t*)p->ast->seq->elements[i])->uint);
	  carray_append(seq, tmp);
	} else {
	  carray_append(seq, p->ast->seq->elements[i]);
	}
      }
      ret->seq = seq;
      return (const parsed_token_t*)ret;
    }
  case TT_UINT:
    {
      parsed_token_t *ret = a_new_(p->arena, parsed_token_t, 1);
      ret->token_type = TT_UINT;
      ret->uint = toupper(p->ast->uint);
      return (const parsed_token_t*)ret;
    }
  default:
    return p->ast;
  }
}

static void test_action(void) {
  const parser_t *action_ = action(sequence(choice(ch('a'), 
						   ch('A'), 
						   NULL), 
					    choice(ch('b'), 
						   ch('B'), 
						   NULL), 
					    NULL), 
				   upcase);

  g_check_parse_ok(action_, "ab", 2, "(u0x41 u0x42)");
  g_check_parse_ok(action_, "AB", 2, "(u0x41 u0x42)");
}

static void test_not_in(void) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const parser_t *not_in_ = not_in(options, 3);
  g_check_parse_ok(not_in_, "d", 1, "u0x64");
  g_check_parse_failed(not_in_, "a", 1);

}

static void test_end_p(void) {
  const parser_t *end_p_ = sequence(ch('a'), end_p(), NULL);
  g_check_parse_ok(end_p_, "a", 1, "(u0x61)");
  g_check_parse_failed(end_p_, "aa", 2);
}

static void test_nothing_p(void) {
  uint8_t test[1] = { 'a' };
  const parser_t *nothing_p_ = nothing_p();
  parse_result_t *ret = parse(nothing_p_, test, 1);
  g_check_failed(ret);
}

static void test_sequence(void) {
  const parser_t *sequence_1 = sequence(ch('a'), ch('b'), NULL);
  const parser_t *sequence_2 = sequence(ch('a'), whitespace(ch('b')), NULL);

  g_check_parse_ok(sequence_1, "ab", 2, "(u0x61 u0x62)");
  g_check_parse_failed(sequence_1, "a", 1);
  g_check_parse_failed(sequence_1, "b", 1);
  g_check_parse_ok(sequence_2, "ab", 2, "(u0x61 u0x62)");
  g_check_parse_ok(sequence_2, "a b", 3, "(u0x61 u0x62)");
  g_check_parse_ok(sequence_2, "a  b", 4, "(u0x61 u0x62)");  
}

static void test_choice(void) {
  const parser_t *choice_ = choice(ch('a'), ch('b'), NULL);

  g_check_parse_ok(choice_, "a", 1, "u0x61");
  g_check_parse_ok(choice_, "b", 1, "u0x62");
  g_check_parse_failed(choice_, "c", 1);
}

static void test_butnot(void) {
  const parser_t *butnot_1 = butnot(ch('a'), token((const uint8_t*)"ab", 2));
  const parser_t *butnot_2 = butnot(ch_range('0', '9'), ch('6'));

  g_check_parse_ok(butnot_1, "a", 1, "u0x61");
  g_check_parse_failed(butnot_1, "ab", 2);
  g_check_parse_ok(butnot_1, "aa", 2, "u0x61");
  g_check_parse_failed(butnot_2, "6", 1);
}

static void test_difference(void) {
  const parser_t *difference_ = difference(token((const uint8_t*)"ab", 2), ch('a'));

  g_check_parse_ok(difference_, "ab", 2, "<61.62>");
  g_check_parse_failed(difference_, "a", 1);
}

static void test_xor(void) {
  const parser_t *xor_ = xor(ch_range('0', '6'), ch_range('5', '9'));

  g_check_parse_ok(xor_, "0", 1, "u0x30");
  g_check_parse_ok(xor_, "9", 1, "u0x39");
  g_check_parse_failed(xor_, "5", 1);
  g_check_parse_failed(xor_, "a", 1);
}

static void test_many(void) {
  const parser_t *many_ = many(choice(ch('a'), ch('b'), NULL));
  g_check_parse_ok(many_, "adef", 4, "(u0x61)");
  g_check_parse_ok(many_, "bdef", 4, "(u0x62)");
  g_check_parse_ok(many_, "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  g_check_parse_ok(many_, "daabbabadef", 11, "()");
}

static void test_many1(void) {
  const parser_t *many1_ = many1(choice(ch('a'), ch('b'), NULL));

  g_check_parse_ok(many1_, "adef", 4, "(u0x61)");
  g_check_parse_ok(many1_, "bdef", 4, "(u0x62)");
  g_check_parse_ok(many1_, "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  g_check_parse_failed(many1_, "daabbabadef", 11);  
}

static void test_repeat_n(void) {
  const parser_t *repeat_n_ = repeat_n(choice(ch('a'), ch('b'), NULL), 2);

  g_check_parse_failed(repeat_n_, "adef", 4);
  g_check_parse_ok(repeat_n_, "abdef", 5, "(u0x61 u0x62)");
  g_check_parse_failed(repeat_n_, "dabdef", 6);
}

static void test_optional(void) {
  const parser_t *optional_ = sequence(ch('a'), optional(choice(ch('b'), ch('c'), NULL)), ch('d'), NULL);
  
  g_check_parse_ok(optional_, "abd", 3, "(u0x61 u0x62 u0x64)");
  g_check_parse_ok(optional_, "acd", 3, "(u0x61 u0x63 u0x64)");
  g_check_parse_ok(optional_, "ad", 2, "(u0x61 null u0x64)");
  g_check_parse_failed(optional_, "aed", 3);
  g_check_parse_failed(optional_, "ab", 2);
  g_check_parse_failed(optional_, "ac", 2);
}

static void test_ignore(void) {
  const parser_t *ignore_ = sequence(ch('a'), ignore(ch('b')), ch('c'), NULL);

  g_check_parse_ok(ignore_, "abc", 3, "(u0x61 u0x63)");
  g_check_parse_failed(ignore_, "ac", 2);
}

static void test_sepBy1(void) {
  const parser_t *sepBy1_ = sepBy1(choice(ch('1'), ch('2'), ch('3'), NULL), ch(','));

  g_check_parse_ok(sepBy1_, "1,2,3", 5, "(u0x31 u0x32 u0x33)");
  g_check_parse_ok(sepBy1_, "1,3,2", 5, "(u0x31 u0x33 u0x32)");
  g_check_parse_ok(sepBy1_, "1,3", 3, "(u0x31 u0x33)");
  g_check_parse_ok(sepBy1_, "3", 1, "(u0x33)");
}

static void test_epsilon_p(void) {
  const parser_t *epsilon_p_1 = sequence(ch('a'), epsilon_p(), ch('b'), NULL);
  const parser_t *epsilon_p_2 = sequence(epsilon_p(), ch('a'), NULL);
  const parser_t *epsilon_p_3 = sequence(ch('a'), epsilon_p(), NULL);
  
  g_check_parse_ok(epsilon_p_1, "ab", 2, "(u0x61 u0x62)");
  g_check_parse_ok(epsilon_p_2, "a", 1, "(u0x61)");
  g_check_parse_ok(epsilon_p_3, "a", 1, "(u0x61)");
}

static void test_attr_bool(void) {

}

static void test_and(void) {
  const parser_t *and_1 = sequence(and(ch('0')), ch('0'), NULL);
  const parser_t *and_2 = sequence(and(ch('0')), ch('1'), NULL);
  const parser_t *and_3 = sequence(ch('1'), and(ch('2')), NULL);

  g_check_parse_ok(and_1, "0", 1, "(u0x30)");
  g_check_parse_failed(and_2, "0", 1);
  g_check_parse_ok(and_3, "12", 2, "(u0x31)");
}

static void test_not(void) {
  const parser_t *not_1 = sequence(ch('a'), choice(ch('+'), token((const uint8_t*)"++", 2), NULL), ch('b'), NULL);
  const parser_t *not_2 = sequence(ch('a'),
				   choice(sequence(ch('+'), not(ch('+')), NULL),
					  token((const uint8_t*)"++", 2),
					  NULL), ch('b'), NULL);

  g_check_parse_ok(not_1, "a+b", 3, "(u0x61 u0x2b u0x62)");
  g_check_parse_failed(not_1, "a++b", 4);
  g_check_parse_ok(not_2, "a+b", 3, "(u0x61 (u0x2b) u0x62)");
  g_check_parse_ok(not_2, "a++b", 4, "(u0x61 <2b.2b> u0x62)");
}

void register_parser_tests(void) {
  g_test_add_func("/core/parser/token", test_token);
  g_test_add_func("/core/parser/ch", test_ch);
  g_test_add_func("/core/parser/ch_range", test_ch_range);
  g_test_add_func("/core/parser/int64", test_int64);
  g_test_add_func("/core/parser/int32", test_int32);
  g_test_add_func("/core/parser/int16", test_int16);
  g_test_add_func("/core/parser/int8", test_int8);
  g_test_add_func("/core/parser/uint64", test_uint64);
  g_test_add_func("/core/parser/uint32", test_uint32);
  g_test_add_func("/core/parser/uint16", test_uint16);
  g_test_add_func("/core/parser/uint8", test_uint8);
#if 0
  g_test_add_func("/core/parser/float64", test_float64);
  g_test_add_func("/core/parser/float32", test_float32);
#endif
  g_test_add_func("/core/parser/whitespace", test_whitespace);
  g_test_add_func("/core/parser/action", test_action);
  g_test_add_func("/core/parser/not_in", test_not_in);
  g_test_add_func("/core/parser/end_p", test_end_p);
  g_test_add_func("/core/parser/nothing_p", test_nothing_p);
  g_test_add_func("/core/parser/sequence", test_sequence);
  g_test_add_func("/core/parser/choice", test_choice);
  g_test_add_func("/core/parser/butnot", test_butnot);
  g_test_add_func("/core/parser/difference", test_difference);
  g_test_add_func("/core/parser/xor", test_xor);
  g_test_add_func("/core/parser/many", test_many);
  g_test_add_func("/core/parser/many1", test_many1);
  g_test_add_func("/core/parser/repeat_n", test_repeat_n);
  g_test_add_func("/core/parser/optional", test_optional);
  g_test_add_func("/core/parser/sepBy1", test_sepBy1);
  g_test_add_func("/core/parser/epsilon_p", test_epsilon_p);
  g_test_add_func("/core/parser/attr_bool", test_attr_bool);
  g_test_add_func("/core/parser/and", test_and);
  g_test_add_func("/core/parser/not", test_not);
  g_test_add_func("/core/parser/ignore", test_ignore);
}

#endif // #ifdef INCLUDE_TESTS
