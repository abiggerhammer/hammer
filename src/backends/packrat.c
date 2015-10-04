#include <assert.h>
#include <string.h>
#include "../internal.h"
#include "../parsers/parser_internal.h"

// short-hand for creating lowlevel parse cache values (parse result case)
static
HParserCacheValue * cached_result(HParseState *state, HParseResult *result) {
  HParserCacheValue *ret = a_new(HParserCacheValue, 1);
  ret->value_type = PC_RIGHT;
  ret->right = result;
  ret->input_stream = state->input_stream;
  return ret;
}

// short-hand for creating lowlevel parse cache values (left recursion case)
static
HParserCacheValue *cached_lr(HParseState *state, HLeftRec *lr) {
  HParserCacheValue *ret = a_new(HParserCacheValue, 1);
  ret->value_type = PC_LEFT;
  ret->left = lr;
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
	size_t bit_length = h_input_stream_pos(&state->input_stream) - h_input_stream_pos(&bak);
	if (tmp_res->bit_length == 0) { // Don't modify if forwarding.
	  tmp_res->bit_length = bit_length;
	}
	if (tmp_res->ast && tmp_res->ast->bit_length != 0) {
	  ((HParsedToken*)(tmp_res->ast))->bit_length = bit_length;
	}
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
  HRecursionHead *head = h_hashtable_get(state->recursion_heads, &k->input_pos);
  if (!head) { // No heads found
    return cached;
  } else { // Some heads found
    if (!cached && head->head_parser != k->parser && !h_slist_find(head->involved_set, k->parser)) {
      // Nothing in the cache, and the key parser is not involved
      cached = cached_result(state, NULL);
      cached->input_stream = k->input_pos;
    }
    if (h_slist_find(head->eval_set, k->parser)) {
      // Something is in the cache, and the key parser is in the eval set. Remove the key parser from the eval set of the head. 
      head->eval_set = h_slist_remove_all(head->eval_set, k->parser);
      HParseResult *tmp_res = perform_lowlevel_parse(state, k->parser);
      // update the cache
      if (!cached) {
	cached = cached_result(state, tmp_res);
	h_hashtable_put(state->cache, k, cached);
      } else {
	cached->value_type = PC_RIGHT;
	cached->right = tmp_res;
	cached->input_stream = state->input_stream;
      }
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
    some->head_parser = p;
    some->involved_set = h_slist_new(state->arena);
    some->eval_set = NULL;
    rec_detect->head = some;
  }

  HSlistNode *it;
  for(it=state->lr_stack->head; it; it=it->next) {
    HLeftRec *lr = it->elem;

    if(lr->rule == p)
      break;

    lr->head = rec_detect->head;
    h_slist_push(lr->head->involved_set, (void*)lr->rule);
  }
}

// helper: true iff pos1 is less than pos2
static inline bool pos_lt(HInputStream pos1, HInputStream pos2) {
  return ((pos1.index < pos2.index) ||
	  (pos1.index == pos2.index && pos1.bit_offset < pos2.bit_offset));
}

/* If recall() returns NULL, we need to store a dummy failure in the cache and compute the
 * future parse. 
 */

HParseResult* grow(HParserCacheKey *k, HParseState *state, HRecursionHead *head) {
  // Store the head into the recursion_heads
  h_hashtable_put(state->recursion_heads, &k->input_pos, head);
  HParserCacheValue *old_cached = h_hashtable_get(state->cache, k);
  if (!old_cached || PC_LEFT == old_cached->value_type)
    h_platform_errx(1, "impossible match");
  HParseResult *old_res = old_cached->right;

  // rewind the input
  state->input_stream = k->input_pos;
  
  // reset the eval_set of the head of the recursion at each beginning of growth
  head->eval_set = h_slist_copy(head->involved_set);
  HParseResult *tmp_res = perform_lowlevel_parse(state, k->parser);

  if (tmp_res) {
    if (pos_lt(old_cached->input_stream, state->input_stream)) {
      h_hashtable_put(state->cache, k, cached_result(state, tmp_res));
      return grow(k, state, head);
    } else {
      // we're done with growing, we can remove data from the recursion head
      h_hashtable_del(state->recursion_heads, &k->input_pos);
      HParserCacheValue *cached = h_hashtable_get(state->cache, k);
      if (cached && PC_RIGHT == cached->value_type) {
        state->input_stream = cached->input_stream;
	return cached->right;
      } else {
	h_platform_errx(1, "impossible match");
      }
    }
  } else {
    h_hashtable_del(state->recursion_heads, &k->input_pos);
    state->input_stream = old_cached->input_stream;
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
      h_hashtable_put(state->cache, k, cached_result(state, growable->seed));
      if (!growable->seed)
	return NULL;
      else
	return grow(k, state, growable->head);
    }
  } else {
    h_platform_errx(1, "lrAnswer with no head");
  }
}

/* Warth's recursion. Hi Alessandro! */
HParseResult* h_do_parse(const HParser* parser, HParseState *state) {
  HParserCacheKey *key = a_new(HParserCacheKey, 1);
  key->input_pos = state->input_stream; key->parser = parser;
  HParserCacheValue *m = NULL;
  if (parser->vtable->higher) {
    m = recall(key, state);
  }
  // check to see if there is already a result for this object...
  if (!m) {
    // It doesn't exist, so create a dummy result to cache
    HLeftRec *base = a_new(HLeftRec, 1);
    // But only cache it now if there's some chance it could grow; primitive parsers can't
    if (parser->vtable->higher) {
      base->seed = NULL; base->rule = parser; base->head = NULL;
      h_slist_push(state->lr_stack, base);
      // cache it
      h_hashtable_put(state->cache, key, cached_lr(state, base));
      // parse the input
    }
    HParseResult *tmp_res = perform_lowlevel_parse(state, parser);
    if (parser->vtable->higher) {
      // the base variable has passed equality tests with the cache
      h_slist_pop(state->lr_stack);
      // update the cached value to our new position
      HParserCacheValue *cached = h_hashtable_get(state->cache, key);
      assert(cached != NULL);
      cached->input_stream = state->input_stream;
    }
    // setupLR, used below, mutates the LR to have a head if appropriate, so we check to see if we have one
    if (NULL == base->head) {
      h_hashtable_put(state->cache, key, cached_result(state, tmp_res));
      return tmp_res;
    } else {
      base->seed = tmp_res;
      HParseResult *res = lr_answer(key, state, base);
      return res;
    }
  } else {
    // it exists!
    state->input_stream = m->input_stream;
    if (PC_LEFT == m->value_type) {
      setupLR(parser, state, m->left);
      return m->left->seed;
    } else {
      return m->right;
    }
  }
}

int h_packrat_compile(HAllocator* mm__, HParser* parser, const void* params) {
  parser->backend = PB_PACKRAT;
  return 0; // No compilation necessary, and everything should work
	    // out of the box.
}

void h_packrat_free(HParser *parser) {
  parser->backend = PB_PACKRAT; // revert to default, oh that's us
}

static uint32_t cache_key_hash(const void* key) {
  return h_djbhash(key, sizeof(HParserCacheKey));
}
static bool cache_key_equal(const void* key1, const void* key2) {
  return memcmp(key1, key2, sizeof(HParserCacheKey)) == 0;
}

static uint32_t pos_hash(const void* key) {
  return h_djbhash(key, sizeof(HInputStream));
}

static bool pos_equal(const void* key1, const void* key2) {
  return memcmp(key1, key2, sizeof(HInputStream)) == 0;
}

HParseResult *h_packrat_parse(HAllocator* mm__, const HParser* parser, HInputStream *input_stream) {
  HArena * arena = h_new_arena(mm__, 0);
  HParseState *parse_state = a_new_(arena, HParseState, 1);
  parse_state->cache = h_hashtable_new(arena, cache_key_equal, // key_equal_func
				       cache_key_hash); // hash_func
  parse_state->input_stream = *input_stream;
  parse_state->lr_stack = h_slist_new(arena);
  parse_state->recursion_heads = h_hashtable_new(arena, pos_equal, pos_hash);
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

HParserBackendVTable h__packrat_backend_vtable = {
  .compile = h_packrat_compile,
  .parse = h_packrat_parse,
  .free = h_packrat_free
};
