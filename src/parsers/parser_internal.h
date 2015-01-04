/*
 * NOTE: This is an internal header and installed for use by extensions. The
 * API is not guaranteed stable.
*/

#ifndef HAMMER_PARSE_INTERNAL__H
#define HAMMER_PARSE_INTERNAL__H
#include "../hammer.h"
#include "../internal.h"
#include "../backends/regex.h"
#include "../backends/contextfree.h"

#define a_new_(arena, typ, count) ((typ*)h_arena_malloc((arena), sizeof(typ)*(count)))
#define a_new(typ, count) a_new_(state->arena, typ, count)
// we can create a_new0 if necessary. It would allocate some memory and immediately zero it out.

static inline HParseResult* make_result(HArena *arena, HParsedToken *tok) {
  HParseResult *ret = h_arena_malloc(arena, sizeof(HParseResult));
  ret->ast = tok;
  ret->arena = arena;
  ret->bit_length = 0; // This way it gets overridden in h_do_parse
  return ret;
}

// return token size in bits...
static inline size_t token_length(HParseResult *pr) {
  if (pr) {
    return pr->bit_length;
  } else {
    return 0;
  }
}

/* Epsilon rules happen during desugaring. This handles them. */
static inline void desugar_epsilon(HAllocator *mm__, HCFStack *stk__, void *env) {
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = h_act_ignore;
  } HCFS_END_CHOICE();
}

#endif // HAMMER_PARSE_INTERNAL__H
