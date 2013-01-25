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

// Standard short-hand for arena-allocating a variable in a semantic action.
#define H_ALLOC(TYP)  ((TYP *) h_arena_malloc(p->arena, sizeof(TYP)))

// Token constructors...

HParsedToken *h_make(HArena *arena, HTokenType type, void *value);
HParsedToken *h_make_seq(HArena *arena);  // Makes empty sequence.
HParsedToken *h_make_bytes(HArena *arena, size_t len);
HParsedToken *h_make_sint(HArena *arena, int64_t val);
HParsedToken *h_make_uint(HArena *arena, uint64_t val);

// Standard short-hands to make tokens in an action.
#define H_MAKE(TYP, VAL)  h_make(p->arena, TT_ ## TYP, VAL)
#define H_MAKE_SEQ()      h_make_seq(p->arena)
#define H_MAKE_BYTES(LEN) h_make_bytes(p->arena, LEN)
#define H_MAKE_SINT(VAL)  h_make_sint(p->arena, VAL)
#define H_MAKE_UINT(VAL)  h_make_uint(p->arena, VAL)

// Extract type-specific value back from HParsedTokens...

void *         h_cast      (HTokenType type, const HParsedToken *p);
HCountedArray *h_cast_seq  (const HParsedToken *p);
HBytes         h_cast_bytes(const HParsedToken *p);
int64_t        h_cast_sint (const HParsedToken *p);
uint64_t       h_cast_uint (const HParsedToken *p);

// Standard short-hand to cast to a user type.
#define H_CAST(TYP, TOK)  ((TYP *) h_cast(TT_ ## TYP, TOK))

// Sequence access...

// Access a sequence element by index.
HParsedToken * h_seq_index(const HParsedToken *p, size_t i);

// Convenience functions combining index access and h_cast_*.
HCountedArray *h_seq_index_seq  (const HParsedToken *p, size_t i);
HBytes         h_seq_index_bytes(const HParsedToken *p, size_t i);
int64_t        h_seq_index_sint (const HParsedToken *p, size_t i);
uint64_t       h_seq_index_uint (const HParsedToken *p, size_t i);
void *         h_seq_index_user (HTokenType type, const HParsedToken *p, size_t i);

// Standard short-hand to access and cast a user-type sequence element.
#define H_INDEX(TYP, SEQ, IDX) \
  ((TYP *) h_seq_index_user(TT_ ## TYP, SEQ, IDX))

// Standard short-hand to access and cast a user-type field on a sequence token.
#define H_FIELD(TYP, IDX)  H_INDEX(TYP, p->ast, IDX)

// Lower-level helper for h_seq_index.
HParsedToken *h_carray_index(const HCountedArray *a, size_t i); // XXX -> internal

// Sequence modification...

// Add elements to a sequence.
void h_seq_snoc(HParsedToken *xs, const HParsedToken *x);     // append one
void h_seq_append(HParsedToken *xs, const HParsedToken *ys);  // append many

// XXX TODO: Remove elements from a sequence.

// Flatten nested sequences into one.
const HParsedToken *h_seq_flatten(HArena *arena, const HParsedToken *p);


#endif
