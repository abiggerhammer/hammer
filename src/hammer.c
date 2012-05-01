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
#include <assert.h>
#include <string.h>
/* TODO(thequux): rewrite to follow new parse_state_t layout
parse_state_t* from(parse_state_t *ps, const size_t index) {
  parse_state_t p = { ps->input, ps->index + index, ps->length - index, ps->cache };
  parse_state_t *ret = g_new(parse_state_t, 1);
  *ret = p;
  return ret;
}
*/
const uint8_t* substring(const parse_state_t *ps, const size_t start, const size_t end) {
  if (end > start && (ps->input_stream.index + end) < ps->input_stream.length) {
    gpointer ret = g_malloc(end - start);
    memcpy(ret, ps->input_stream.input, end - start);
    return (const uint8_t*)ret;
  } else {
    return NULL;
  }
}

const GVariant* at(parse_state_t *ps, const size_t index) {
  GVariant *ret = NULL;
  if (index + ps->input_stream.index < ps->input_stream.length) 
    ret = g_variant_new_byte((ps->input_stream.input)[index + ps->input_stream.index]);
  return g_variant_new_maybe(G_VARIANT_TYPE_BYTE, ret);
}

const gchar* to_string(parse_state_t *ps) {
  return g_strescape((const gchar*)(ps->input_stream.input), NULL);
}

uint8_t djbhash(size_t index, char bit_offset) {
  unsigned int hash = 5381;
  for (uint8_t i = 0; i < sizeof(size_t); ++i) {
    hash = hash * 33 + (index & 0xFF);
    index >>= 8;
  }
  hash = hash * 33 + bit_offset;
  return hash;
}

parse_result_t* get_cached(parse_state_t *ps, const parser_t *p) {
  gpointer t = g_hash_table_lookup(ps->cache, p);
  if (NULL != t) {
    parse_result_t* ret = g_hash_table_lookup(t, GUINT_TO_POINTER(djbhash(ps->input_stream.index, 
									  ps->input_stream.length)));
    if (NULL != ret) {
      return ret;
    } else {
      // TODO(mlp): need a return value for "this parser was in the cache but nothing was at this location"
      return NULL;
    }
  } else {
    // TODO(mlp): need a return value for "this parser wasn't in the cache"
    return NULL;
  }
}

void put_cached(parse_state_t *ps, const parser_t *p, parse_result_t *cached) {
  gpointer t = g_hash_table_lookup(ps->cache, p);
  if (NULL != t) {
    g_hash_table_insert(t, GUINT_TO_POINTER(djbhash(ps->input_stream.index, ps->input_stream.length)), (gpointer)cached); 
  } else {
    GHashTable *t = g_hash_table_new(g_direct_hash, g_direct_equal);
    g_hash_table_insert(t, GUINT_TO_POINTER(djbhash(ps->input_stream.index, ps->input_stream.length)), (gpointer)cached);
    g_hash_table_insert(ps->cache, (parser_t*)p, t);
  }
}

parse_result_t* do_parse(const parser_t* parser, parse_state_t *state);

/* Helper function, since these lines appear in every parser */
inline parse_result_t* make_result(parsed_token_t *tok) {
  parse_result_t *ret = g_new(parse_result_t, 1);
  ret->ast = tok;
  return ret;
}

typedef struct {
  uint8_t *str;
  uint8_t len;
} token_t;

static parse_result_t* parse_token(void *env, parse_state_t *state) {
  token_t *t = (token_t*)env;
  for (int i=0; i<t->len; ++i) {
    uint8_t chr = (uint8_t)read_bits(&state->input_stream, 8, false);
    if (t->str[i] != chr) {
      return NULL;
    }
  }
  parsed_token_t *tok = g_new(parsed_token_t, 1);
  tok->token_type = TT_BYTES; tok->bytes.token = t->str; tok->bytes.len = t->len;
  return make_result(tok);
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
    parsed_token_t *tok = g_new(parsed_token_t, 1);    
    tok->token_type = TT_UINT; tok->uint = r;
    return make_result(tok);
  } else {
    return NULL;
  }
}

const parser_t* ch(const uint8_t c) {  
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_ch; ret->env = GUINT_TO_POINTER(c);
  return (const parser_t*)ret;
}

typedef struct {
  uint8_t lower;
  uint8_t upper;
} range_t;

static parse_result_t* parse_range(void* env, parse_state_t *state) {
  range_t *range = (range_t*)env;
  uint8_t r = (uint8_t)read_bits(&state->input_stream, 8, false);
  if (range->lower <= r && range->upper >= r) {
    parsed_token_t *tok = g_new(parsed_token_t, 1);
    tok->token_type = TT_UINT; tok->uint = r;
    return make_result(tok);
  } else {
    return NULL;
  }
}

const parser_t* range(const uint8_t lower, const uint8_t upper) { 
  range_t *r = g_new(range_t, 1);
  r->lower = lower; r->upper = upper;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_range; ret->env = (void*)r;
  return (const parser_t*)ret;
}
const parser_t* whitespace(const parser_t* p) { return NULL; }
//const parser_t* action(const parser_t* p, /* fptr to action on AST */) { return NULL; }

const parser_t* left_factor_action(const parser_t* p) { return NULL; }

static parse_result_t* parse_negate(void *env, parse_state_t *state) {
  parser_t *p = (parser_t*)env;
  parse_result_t *result = do_parse(p, state);
  if (NULL == result) {
    uint8_t r = (uint8_t)read_bits(&state->input_stream, 8, false);
    parsed_token_t *tok = g_new(parsed_token_t, 1);    
    tok->token_type = TT_UINT; tok->uint = r;
    return make_result(tok);    
  } else {
    return NULL;
  }
}

const parser_t* negate(const parser_t* p) { 
  assert(parse_ch == p->fn || parse_range == p->fn);
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_negate; ret->env = (void*)p;
  return (const parser_t*)ret;
}

static parse_result_t* parse_end(void *env, parse_state_t *state) {
  if (state->input_stream.index == state->input_stream.length) {
    parse_result_t *ret = g_new(parse_result_t, 1);
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
const parser_t* nothing_p() { 
  // not a mistake, this parser always fails
  return NULL; 
}

typedef struct {
  size_t len;
  const parser_t **p_array;
} sequence_t;

static parse_result_t* parse_sequence(void *env, parse_state_t *state) {
  sequence_t *s = (sequence_t*)env;
  GSequence *seq = g_sequence_new(NULL);
  for (size_t i=0; i<s->len; ++i) {
    parse_result_t *tmp = do_parse(s->p_array[i], state);
    g_sequence_append(seq, tmp);
  }
  parsed_token_t *tok = g_new(parsed_token_t, 1);
  tok->token_type = TT_SEQUENCE; tok->seq = seq;
  return make_result(tok);
}

const parser_t* sequence(const parser_t* p_array[]) { 
  size_t len = sizeof(p_array) / sizeof(parser_t*);
  sequence_t *s = g_new(sequence_t, 1);
  s->p_array = (const parser_t**)p_array; s->len = len;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_sequence; ret->env = (void*)s;
  return ret;
}

static parse_result_t* parse_choice(void *env, parse_state_t *state) {
  sequence_t *s = (sequence_t*)env;
  for (size_t i=0; i<s->len; ++i) {
    parse_result_t *tmp = do_parse(s->p_array[i], state);
    if (NULL != tmp)
      return tmp;
  }
  // nothing succeeded, so fail
  return NULL;
}

const parser_t* choice(const parser_t* p_array[]) { 
  size_t len = sizeof(p_array) / sizeof(parser_t*);
  sequence_t *s = g_new(sequence_t, 1);
  s->p_array = (const parser_t**)p_array; s->len = len;
  parser_t *ret = g_new(parser_t, 1);
  ret->fn = parse_choice; ret->env = (void*)s;
  return ret;
}

typedef struct {
  const parser_t *p1;
  const parser_t *p2;
} two_parsers_t;


static parse_result_t parse_butnot(void *env, parse_state_t *state) {
  two_parsers_t *parsers = (two_parsers_t*)env;
  input_stream_t tmp_state = state->input_stream;
  
}

const parser_t* butnot(const parser_t* p1, const parser_t* p2) { 
  two_parsers_t *env = g_new(two_parsers_t, 1);
  env->p1 = p1; env->p2 = p2;
}

const parser_t* difference(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* xor(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* repeat0(const parser_t* p) { return NULL; }
const parser_t* repeat1(const parser_t* p) { return NULL; }
const parser_t* repeat_n(const parser_t* p, const size_t n) { return NULL; }
const parser_t* optional(const parser_t* p) { return NULL; }
const parser_t* expect(const parser_t* p) { return NULL; }
const parser_t* chain(const parser_t* p1, const parser_t* p2, const parser_t* p3) { return NULL; }
const parser_t* chainl(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* list(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* epsilon_p() { return NULL; }
//const parser_t* semantic(/* fptr to nullary function? */) { return NULL; }
const parser_t* and(const parser_t* p) { return NULL; }
const parser_t* not(const parser_t* p) { return NULL; }

parse_result_t* parse(const parser_t* parser, const uint8_t* input) { return NULL; }
