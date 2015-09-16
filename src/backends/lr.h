#ifndef HAMMER_BACKENDS_LR__H
#define HAMMER_BACKENDS_LR__H

#include "../hammer.h"
#include "../cfgrammar.h"
#include "../internal.h"


typedef HHashSet HLRState;  // states are sets of LRItems

typedef struct HLRDFA_ {
  size_t nstates;
  const HLRState **states;  // array of size nstates
  HSlist *transitions;
} HLRDFA;

typedef struct HLRTransition_ {
  size_t from;              // index into 'states' array
  const HCFChoice *symbol;
  size_t to;                // index into 'states' array
} HLRTransition;

typedef struct HLRItem_ {
  HCFChoice *lhs;
  HCFChoice **rhs;          // NULL-terminated
  size_t len;               // number of elements in rhs
  size_t mark;
} HLRItem;

typedef struct HLRAction_ {
  enum {HLR_SHIFT, HLR_REDUCE, HLR_CONFLICT} type;
  union {
    // used with HLR_SHIFT
    size_t nextstate;

    // used with HLR_REDUCE
    struct {
      HCFChoice *lhs;   // symbol carrying semantic actions etc.
      size_t length;    // # of symbols in rhs
#ifndef NDEBUG
      HCFChoice **rhs;  // NB: the rhs symbols are not needed for the parse
#endif
    } production;

    // used with HLR_CONFLICT
    HSlist *branches;   // list of possible HLRActions
  };
} HLRAction;

typedef struct HLRTable_ {
  size_t     nrows;     // dimension of the pointer arrays below
  HHashTable **ntmap;   // map nonterminal symbols to HLRActions, per row
  HStringMap **tmap;    // map lookahead strings to HLRActions, per row
  HLRAction  **forall;  // shortcut to set an action for an entire row
  HCFChoice  *start;    // start symbol
  HSlist     *inadeq;   // indices of any inadequate states
  HArena     *arena;
  HAllocator *mm__;
} HLRTable;

typedef struct HLREnhGrammar_ {
  HCFGrammar *grammar;  // enhanced grammar
  HHashTable *tmap;     // maps transitions to enhanced-grammar symbols
  HHashTable *smap;     // maps enhanced-grammar symbols to transitions
  HHashTable *corr;     // maps symbols to sets of corresponding e. symbols
  HArena *arena;
} HLREnhGrammar;

typedef struct HLREngine_ {
  const HLRTable *table;
  size_t state;

  HSlist *stack;        // holds pairs: (saved state, semantic value)
  HInputStream input;

  struct HLREngine_ *merged[2]; // ancestors merged into this engine

  HArena *arena;        // will hold the results
  HArena *tarena;       // tmp, deleted after parse
} HLREngine;

#define HLR_SUCCESS ((size_t)~0)    // parser end state


// XXX move to internal.h or something
// XXX replace other hashtable iterations with this
#define H_FOREACH_(HT) {                                                    \
    const HHashTable *ht__ = HT;                                            \
    for(size_t i__=0; i__ < ht__->capacity; i__++) {                        \
      for(HHashTableEntry *hte__ = &ht__->contents[i__];                    \
          hte__;                                                            \
          hte__ = hte__->next) {                                            \
        if(hte__->key == NULL) continue;

#define H_FOREACH_KEY(HT, KEYVAR) H_FOREACH_(HT)                            \
        const KEYVAR = hte__->key;

#define H_FOREACH(HT, KEYVAR, VALVAR) H_FOREACH_KEY(HT, KEYVAR)             \
        VALVAR = hte__->value;

#define H_END_FOREACH                                                       \
      }                                                                     \
    }                                                                       \
  }



HLRItem *h_lritem_new(HArena *a, HCFChoice *lhs, HCFChoice **rhs, size_t mark);
HLRState *h_lrstate_new(HArena *arena);
HLRTable *h_lrtable_new(HAllocator *mm__, size_t nrows);
void h_lrtable_free(HLRTable *table);
HLREngine *h_lrengine_new(HArena *arena, HArena *tarena, const HLRTable *table,
                          const HInputStream *stream);
HLRAction *h_reduce_action(HArena *arena, const HLRItem *item);
HLRAction *h_shift_action(HArena *arena, size_t nextstate);
HLRAction *h_lr_conflict(HArena *arena, HLRAction *action, HLRAction *new);
bool h_lrtable_row_empty(const HLRTable *table, size_t i);

bool h_eq_symbol(const void *p, const void *q);
bool h_eq_lr_itemset(const void *p, const void *q);
bool h_eq_transition(const void *p, const void *q);
HHashValue h_hash_symbol(const void *p);
HHashValue h_hash_lr_itemset(const void *p);
HHashValue h_hash_transition(const void *p);

HLRDFA *h_lr0_dfa(HCFGrammar *g);
HLRTable *h_lr0_table(HCFGrammar *g, const HLRDFA *dfa);

HCFChoice *h_desugar_augmented(HAllocator *mm__, HParser *parser);
int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params);
void h_lalr_free(HParser *parser);

const HLRAction *h_lrengine_action(const HLREngine *engine);
bool h_lrengine_step(HLREngine *engine, const HLRAction *action);
HParseResult *h_lrengine_result(HLREngine *engine);
HParseResult *h_lr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream);
void h_lr_parse_start(HSuspendedParser *s);
bool h_lr_parse_chunk(HSuspendedParser* s, HInputStream *stream);
HParseResult *h_lr_parse_finish(HSuspendedParser *s);
HParseResult *h_glr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream);

void h_pprint_lritem(FILE *f, const HCFGrammar *g, const HLRItem *item);
void h_pprint_lrstate(FILE *f, const HCFGrammar *g,
                      const HLRState *state, unsigned int indent);
void h_pprint_lrdfa(FILE *f, const HCFGrammar *g,
                    const HLRDFA *dfa, unsigned int indent);
void h_pprint_lrtable(FILE *f, const HCFGrammar *g, const HLRTable *table,
                      unsigned int indent);
HCFGrammar *h_pprint_lr_info(FILE *f, HParser *p);

#endif
