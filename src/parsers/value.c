#include "parser_internal.h"

typedef struct {
  const HParser* p;
  const char* key;
} HStoredValue;

/* Stash an HParseResult into a symbol table, so that it can be 
   retrieved and used later. */

static HParseResult* parse_put(void *env, HParseState* state) {
  HStoredValue *s = (HStoredValue*)env;
  if (s->p && s->key && !h_symbol_get(state, s->key)) {
    HParseResult *tmp = h_do_parse(s->p, state);
    if (tmp) {
      h_symbol_put(state, s->key, tmp);
    }
    return tmp;
  } 
  // otherwise there's no parser, no key, or key's stored already
  return NULL;
}

static const HParserVtable put_vt = {
  .parse = parse_put,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .compile_to_rvm = h_not_regular,
  .higher = true,
};

HParser* h_put_value(const HParser* p, const char* name) {
  return h_put_value__m(&system_allocator, p, name);
}

HParser* h_put_value__m(HAllocator* mm__, const HParser* p, const char* name) {
  HStoredValue *env = h_new(HStoredValue, 1);
  env->p = p;
  env->key = name;
  return h_new_parser(mm__, &put_vt, env);
}

/* Retrieve a stashed result from the symbol table. */

static HParseResult* parse_get(void *env, HParseState* state) {
  HStoredValue *s = (HStoredValue*)env;
  if (!s->p && s->key) {
    return h_symbol_get(state, s->key);
  } else { // either there's no key, or there was a parser here
    return NULL;
  }
}

static const HParserVtable get_vt = {
  .parse = parse_get,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .compile_to_rvm = h_not_regular,
  .higher = true,
};

HParser* h_get_value(const char* name) {
  return h_get_value__m(&system_allocator, name);
}

HParser* h_get_value__m(HAllocator* mm__, const char* name) {
  HStoredValue *env = h_new(HStoredValue, 1);
  env->p = NULL;
  env->key = name;
  return h_new_parser(mm__, &get_vt, env);
}
