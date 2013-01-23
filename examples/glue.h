//
// API additions for writing grammar and semantic actions more concisely
//

#ifndef HAMMER_EXAMPLES_GLUE__H
#define HAMMER_EXAMPLES_GLUE__H

#include <assert.h>
#include "../src/hammer.h"


//
// Grammar specification
//

#define H_RULE(rule, def) const HParser *rule = def
#define H_ARULE(rule, def) const HParser *rule = h_action(def, act_ ## rule)
#define H_VRULE(rule, def) const HParser *rule = \
  h_attr_bool(def, validate_ ## rule)
#define H_VARULE(rule, def) const HParser *rule = \
  h_attr_bool(h_action(def, act_ ## rule), validate_ ## rule)
#define H_AVRULE(rule, def) const HParser *rule = \
  h_action(h_attr_bool(def, validate_ ## rule), act_ ## rule)


//
// Pre-fab semantic actions
//

const HParsedToken *h_act_ignore(const HParseResult *p);
const HParsedToken *h_act_index(int i, const HParseResult *p);
const HParsedToken *h_act_flatten(const HParseResult *p);

// Define 'myaction' as a specialization of 'paction' by supplying the leading
// parameters.
#define H_ACT_APPLY(myaction, paction, ...) \
  const HParsedToken *myaction(const HParseResult *p) { \
    return paction(__VA_ARGS__, p); \
  }


//
// Working with HParsedTokens
//

// Token constructors...

HParsedToken *h_make(HArena *arena, HTokenType type, void *value);
HParsedToken *h_make_seq(HArena *arena);

#define H_ALLOC(TYP) \
  ((TYP *) h_arena_malloc(p->arena, sizeof(TYP)))

#define H_MAKE(TYP, VAL) \
  h_make(p->arena, TT_ ## TYP, VAL)

// Sequences...

// Flatten nested sequences into one.
const HParsedToken *h_seq_flatten(HArena *arena, const HParsedToken *p);

void h_seq_snoc(HParsedToken *xs, const HParsedToken *x);
void h_seq_append(HParsedToken *xs, const HParsedToken *ys);

HParsedToken *h_carray_index(const HCountedArray *a, size_t i); // XXX -> internal
HParsedToken *h_seq_index(const HParsedToken *p, size_t i);
void *h_seq_index_user(HTokenType type, const HParsedToken *p, size_t i);

#define H_SEQ_INDEX(TYP, SEQ, IDX) \
  ((TYP *) h_seq_index_user(TT_ ## TYP, SEQ, IDX))

#define H_FIELD(TYP, IDX) \
  H_SEQ_INDEX(TYP, p->ast, IDX)


#endif
