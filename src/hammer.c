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

parse_state_t* from(parse_state_t *ps, const size_t index) {
  parse_state_t *ret = g_new(parse_state_t, 1);
  *ret = *ps;
  ret->input_stream.index += index;
  return ret;
}

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

guint djbhash(const uint8_t *buf, size_t len) {
  guint hash = 5381;
  while (len--) {
    hash = hash * 33 + *buf++;
  }
  return hash;
}

parse_result_t* do_parse(const parser_t* parser, parse_state_t *state) {
  // TODO(thequux): add caching here.
  parser_cache_key_t key = {
    .input_pos = state->input_stream,
    .parser = parser
  };
  
  // check to see if there is already a result for this object...
  if (g_hash_table_contains(state->cache, &key)) {
    // it exists!
    // TODO(thequux): handle left recursion case
    return g_hash_table_lookup(state->cache, &key);
  } else {
    // It doesn't exist... run the 
    parse_result_t *res;
    res = parser->fn(parser->env, state);
    // update the cache
    g_hash_table_replace(state->cache, &key, res);
#ifdef CONSISTENCY_CHECK
    if (!res) {
      state->input_stream = INVALID;
      state->input_stream.input = key.input_pos.input;
    }
#endif
    return res;
  }
}

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

const parser_t* whitespace(const parser_t* p) { return NULL; }
//const parser_t* action(const parser_t* p, /* fptr to action on AST */) { return NULL; }

const parser_t* left_factor_action(const parser_t* p) { return NULL; }

static parse_result_t* parse_charset(void *env, parse_state_t *state) {
  uint8_t in = read_bits(&state->input_stream, 8, false);
  charset cs = (charset)env;

  if (charset_isset(cs, in)) {
    parsed_token_t *tok = g_new(parsed_token_t, 1);
    tok->token_type = TT_UINT; tok->uint = in;
    return make_result(tok);    
  } else
    return NULL;
}

const parser_t* range(const uint8_t lower, const uint8_t upper) {
  parser_t *ret = g_new(parser_t, 1);
  charset cs = new_charset();
  for (int i = 0; i < 256; i++)
    charset_set(cs, i, (lower <= i) && (i <= upper));
  ret->fn = parse_charset; ret->env = (void*)cs;
  return (const parser_t*)ret;
}

const parser_t* notin(const uint8_t *options, int count) {
  parser_t *ret = g_new(parser_t, 1);
  charset cs = new_charset();
  for (int i = 0; i < 256; i++)
    charset_set(cs, i, 1);
  for (int i = 0; i < count; i++)
    charset_set(cs, i, 0);

  ret->fn = parse_charset; ret->env = (void*)cs;
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
    // if the interim parse fails, the whole thing fails
    if (NULL == tmp) {
      return NULL;
    } else {
      g_sequence_append(seq, tmp);
    }
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

void accumulate_size(gpointer pr, gpointer acc) {
  size_t tmp = GPOINTER_TO_SIZE(acc);
  if (NULL != ((parse_result_t*)pr)->ast) {
    switch(((parse_result_t*)pr)->ast->token_type) {
    case TT_BYTES:
      tmp += ((parse_result_t*)pr)->ast->bytes.len;
      acc = GSIZE_TO_POINTER(tmp);
      break;
    case TT_SINT:
    case TT_UINT:
      tmp += 8;
      acc = GSIZE_TO_POINTER(tmp);
      break;
    case TT_SEQUENCE:
      g_sequence_foreach(((parse_result_t*)pr)->ast->seq, accumulate_size, acc);
      break;
    default:
      break;
    }
  } // no else, if the AST is null then acc doesn't change
}

size_t token_length(parse_result_t *pr) {
  size_t ret = 0;
  if (NULL == pr) {
    return ret;
  } else {
    accumulate_size(pr, GSIZE_TO_POINTER(ret));
  }
  return ret;
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
  // if both match but p1's text is longer than p2's, fail
  if (r1len > r2len) {
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

static guint cache_key_hash(gconstpointer key) {
  return djbhash(key, sizeof(parser_cache_key_t));
}
static gboolean cache_key_equal(gconstpointer key1, gconstpointer key2) {
  return memcmp(key1, key2, sizeof(parser_cache_key_t)) == 0;
}


parse_result_t* parse(const parser_t* parser, const uint8_t* input, size_t length) { 
  // Set up a parse state...
  parse_state_t *parse_state = g_new0(parse_state_t, 1);
  parse_state->cache = g_hash_table_new(cache_key_hash, // hash_func
					cache_key_equal);// key_equal_func
  parse_state->input_stream.input = input;
  parse_state->input_stream.bit_offset = 8; // bit big endian
  parse_state->input_stream.endianness = BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN;
  parse_state->input_stream.length = length;
  
  parse_result_t *res = do_parse(parser, parse_state);
  // tear down the parse state. For now, leak like a sieve.
  // BUG: Leaks like a sieve.
  // TODO(thequux): Pull in the arena allocator.
  g_hash_table_destroy(parse_state->cache);

  return res;
}
