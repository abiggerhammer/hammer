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

#include <assert.h>
#include <ctype.h>
#include <error.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include "allocator.h"
#include "parsers/parser_internal.h"

static uint32_t djbhash(const uint8_t *buf, size_t len) {
  uint32_t hash = 5381;
  while (len--) {
    hash = hash * 33 + *buf++;
  }
  return hash;
}

// short-hand for constructing HCachedResult's
static HCachedResult *cached_result(const HParseState *state, HParseResult *result) {
  HCachedResult *ret = a_new(HCachedResult, 1);
  ret->result = result;
  ret->input_stream = state->input_stream;
  return ret;
}

// Really library-internal tool to perform an uncached parse, and handle any common error-handling.
static inline HParseResult* perform_lowlevel_parse(HParseState *state, const HParser *parser) {
  // TODO(thequux): these nested conditions are ugly. Factor this appropriately, so that it is clear which codes is executed when.
  HParseResult *tmp_res;
  if (parser) {
    HInputStream bak = state->input_stream;
    tmp_res = parser->vtable->parse(parser->env, state);
    if (tmp_res) {
      tmp_res->arena = state->arena;
      if (!state->input_stream.overrun) {
	tmp_res->bit_length = ((state->input_stream.index - bak.index) << 3);
	if (state->input_stream.endianness & BIT_BIG_ENDIAN)
	  tmp_res->bit_length += state->input_stream.bit_offset - bak.bit_offset;
	else
	  tmp_res->bit_length += bak.bit_offset - state->input_stream.bit_offset;
      } else
	tmp_res->bit_length = 0;
    }
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
  return tmp_res;
}

HParserCacheValue* recall(HParserCacheKey *k, HParseState *state) {
  HParserCacheValue *cached = h_hashtable_get(state->cache, k);
  HRecursionHead *head = h_hashtable_get(state->recursion_heads, k);
  if (!head) { // No heads found
    return cached;
  } else { // Some heads found
    if (!cached && head->head_parser != k->parser && !h_slist_find(head->involved_set, k->parser)) {
      // Nothing in the cache, and the key parser is not involved
      HParseResult *tmp = a_new(HParseResult, 1);
      tmp->ast = NULL; tmp->arena = state->arena;
      HParserCacheValue *ret = a_new(HParserCacheValue, 1);
      ret->value_type = PC_RIGHT; ret->right = cached_result(state, tmp);
      return ret;
    }
    if (h_slist_find(head->eval_set, k->parser)) {
      // Something is in the cache, and the key parser is in the eval set. Remove the key parser from the eval set of the head. 
      head->eval_set = h_slist_remove_all(head->eval_set, k->parser);
      HParseResult *tmp_res = perform_lowlevel_parse(state, k->parser);
      // we know that cached has an entry here, modify it
      if (!cached)
	cached = a_new(HParserCacheValue, 1);
      cached->value_type = PC_RIGHT;
      cached->right = cached_result(state, tmp_res);
    }
    return cached;
  }
}

/* Setting up the left recursion. We have the LR for the rule head;
 * we modify the involved_sets of all LRs in the stack, until we 
 * see the current parser again.
 */

void setupLR(const HParser *p, HParseState *state, HLeftRec *rec_detect) {
  if (!rec_detect->head) {
    HRecursionHead *some = a_new(HRecursionHead, 1);
    some->head_parser = p; some->involved_set = NULL; some->eval_set = NULL;
    rec_detect->head = some;
  }
  assert(state->lr_stack->head != NULL);
  HLeftRec *lr = state->lr_stack->head->elem;
  while (lr && lr->rule != p) {
    lr->head = rec_detect->head;
    h_slist_push(lr->head->involved_set, (void*)lr->rule);
  }
}

/* If recall() returns NULL, we need to store a dummy failure in the cache and compute the
 * future parse. 
 */

HParseResult* grow(HParserCacheKey *k, HParseState *state, HRecursionHead *head) {
  // Store the head into the recursion_heads
  h_hashtable_put(state->recursion_heads, k, head);
  HParserCacheValue *old_cached = h_hashtable_get(state->cache, k);
  if (!old_cached || PC_LEFT == old_cached->value_type)
    errx(1, "impossible match");
  HParseResult *old_res = old_cached->right->result;
  
  // reset the eval_set of the head of the recursion at each beginning of growth
  head->eval_set = head->involved_set;
  HParseResult *tmp_res = perform_lowlevel_parse(state, k->parser);

  if (tmp_res) {
    if ((old_res->ast->index < tmp_res->ast->index) || 
	(old_res->ast->index == tmp_res->ast->index && old_res->ast->bit_offset < tmp_res->ast->bit_offset)) {
      HParserCacheValue *v = a_new(HParserCacheValue, 1);
      v->value_type = PC_RIGHT; v->right = cached_result(state, tmp_res);
      h_hashtable_put(state->cache, k, v);
      return grow(k, state, head);
    } else {
      // we're done with growing, we can remove data from the recursion head
      h_hashtable_del(state->recursion_heads, k);
      HParserCacheValue *cached = h_hashtable_get(state->cache, k);
      if (cached && PC_RIGHT == cached->value_type) {
	return cached->right->result;
      } else {
	errx(1, "impossible match");
      }
    }
  } else {
    h_hashtable_del(state->recursion_heads, k);
    return old_res;
  }
}

HParseResult* lr_answer(HParserCacheKey *k, HParseState *state, HLeftRec *growable) {
  if (growable->head) {
    if (growable->head->head_parser != k->parser) {
      // not the head rule, so not growing
      return growable->seed;
    }
    else {
      // update cache
      HParserCacheValue *v = a_new(HParserCacheValue, 1);
      v->value_type = PC_RIGHT; v->right = cached_result(state, growable->seed);
      h_hashtable_put(state->cache, k, v);
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
HParseResult* h_do_parse(const HParser* parser, HParseState *state) {
  HParserCacheKey *key = a_new(HParserCacheKey, 1);
  key->input_pos = state->input_stream; key->parser = parser;
  HParserCacheValue *m = recall(key, state);
  // check to see if there is already a result for this object...
  if (!m) {
    // It doesn't exist, so create a dummy result to cache
    HLeftRec *base = a_new(HLeftRec, 1);
    base->seed = NULL; base->rule = parser; base->head = NULL;
    h_slist_push(state->lr_stack, base);
    // cache it
    HParserCacheValue *dummy = a_new(HParserCacheValue, 1);
    dummy->value_type = PC_LEFT; dummy->left = base;
    h_hashtable_put(state->cache, key, dummy);
    // parse the input
    HParseResult *tmp_res = perform_lowlevel_parse(state, parser);
    // the base variable has passed equality tests with the cache
    h_slist_pop(state->lr_stack);
    // setupLR, used below, mutates the LR to have a head if appropriate, so we check to see if we have one
    if (NULL == base->head) {
      HParserCacheValue *right = a_new(HParserCacheValue, 1);
      right->value_type = PC_RIGHT; right->right = cached_result(state, tmp_res);
      h_hashtable_put(state->cache, key, right);
      return tmp_res;
    } else {
      base->seed = tmp_res;
      HParseResult *res = lr_answer(key, state, base);
      return res;
    }
  } else {
    // it exists!
    if (PC_LEFT == m->value_type) {
      setupLR(parser, state, m->left);
      return m->left->seed; // BUG: this might not be correct
    } else {
      state->input_stream = m->right->input_stream;
      return m->right->result;
    }
  }
}

/* Helper function, since these lines appear in every parser */

typedef struct {
  const HParser *p1;
  const HParser *p2;
} HTwoParsers;


static uint32_t cache_key_hash(const void* key) {
  return djbhash(key, sizeof(HParserCacheKey));
}
static bool cache_key_equal(const void* key1, const void* key2) {
  return memcmp(key1, key2, sizeof(HParserCacheKey)) == 0;
}


HParseResult* h_parse(const HParser* parser, const uint8_t* input, size_t length) {
  return h_parse__m(&system_allocator, parser, input, length);
}
HParseResult* h_parse__m(HAllocator* mm__, const HParser* parser, const uint8_t* input, size_t length) { 
  // Set up a parse state...
  HArena * arena = h_new_arena(mm__, 0);
  HParseState *parse_state = a_new_(arena, HParseState, 1);
  parse_state->cache = h_hashtable_new(arena, cache_key_equal, // key_equal_func
					      cache_key_hash); // hash_func
  parse_state->input_stream.input = input;
  parse_state->input_stream.index = 0;
  parse_state->input_stream.bit_offset = 8; // bit big endian
  parse_state->input_stream.overrun = 0;
  parse_state->input_stream.endianness = BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN;
  parse_state->input_stream.length = length;
  parse_state->lr_stack = h_slist_new(arena);
  parse_state->recursion_heads = h_hashtable_new(arena, cache_key_equal,
						  cache_key_hash);
  parse_state->arena = arena;
  HParseResult *res = h_do_parse(parser, parse_state);
  h_slist_free(parse_state->lr_stack);
  h_hashtable_free(parse_state->recursion_heads);
  // tear down the parse state
  h_hashtable_free(parse_state->cache);
  if (!res)
    h_delete_arena(parse_state->arena);

  return res;
}

void h_parse_result_free(HParseResult *result) {
  h_delete_arena(result->arena);
}


