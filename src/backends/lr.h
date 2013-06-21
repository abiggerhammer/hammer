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
  size_t     nrows;
  HHashTable **rows;    // map symbols to HLRActions
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
  bool run;

  // stack layout:
  // on the left stack, we put pairs:  (saved state, semantic value)
  // on the right stack, we put pairs: (symbol, semantic value)
  HSlist *left;     // left stack; reductions happen here
  HSlist *right;    // right stack; input appears here

  HArena *arena;    // will hold the results
  HArena *tarena;   // tmp, deleted after parse
} HLREngine;


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
HLREngine *h_lrengine_new(HArena *arena, HArena *tarena, const HLRTable *table);
HLRAction *h_reduce_action(HArena *arena, const HLRItem *item);
HLRAction *h_shift_action(HArena *arena, size_t nextstate);
HLRAction *h_lr_conflict(HArena *arena, HLRAction *action, HLRAction *new);

bool h_eq_symbol(const void *p, const void *q);
bool h_eq_lr_itemset(const void *p, const void *q);
bool h_eq_transition(const void *p, const void *q);
HHashValue h_hash_symbol(const void *p);
HHashValue h_hash_lr_itemset(const void *p);
HHashValue h_hash_transition(const void *p);

HLRDFA *h_lr0_dfa(HCFGrammar *g);
HLRTable *h_lr0_table(HCFGrammar *g, const HLRDFA *dfa);
int h_lrtable_put(HLRTable *tbl, size_t state, HCFChoice *x, HLRAction *action);

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params);
void h_lalr_free(HParser *parser);

const HLRAction *h_lr_lookup(const HLRTable *table, size_t state, const HCFChoice *symbol);
const HLRAction *h_lrengine_action(HLREngine *engine, HInputStream *stream);
void h_lrengine_step(HLREngine *engine, const HLRAction *action);
HParseResult *h_lrengine_result(HLREngine *engine);
HParseResult *h_lr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream);
HParseResult *h_glr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream);

void h_pprint_lritem(FILE *f, const HCFGrammar *g, const HLRItem *item);
void h_pprint_lrstate(FILE *f, const HCFGrammar *g,
                      const HLRState *state, unsigned int indent);
void h_pprint_lrdfa(FILE *f, const HCFGrammar *g,
                    const HLRDFA *dfa, unsigned int indent);
void h_pprint_lrtable(FILE *f, const HCFGrammar *g, const HLRTable *table,
                      unsigned int indent);

#endif
