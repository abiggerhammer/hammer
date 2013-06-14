#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"



/* Data structures */

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
  enum {HLR_SHIFT, HLR_REDUCE} type;
  union {
    size_t nextstate;   // used with SHIFT
    struct {
      HCFChoice *lhs;   // symbol carrying semantic actions etc.
      size_t length;    // # of symbols in rhs
#ifndef NDEBUG
      HCFChoice **rhs;  // NB: the rhs symbols are not needed for the parse
#endif
    } production;       // used with REDUCE
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


// compare symbols - terminals by value, others by pointer
static bool eq_symbol(const void *p, const void *q)
{
  const HCFChoice *x=p, *y=q;
  return (x==y
          || (x->type==HCF_END && y->type==HCF_END)
          || (x->type==HCF_CHAR && y->type==HCF_CHAR && x->chr==y->chr));
}

// hash symbols - terminals by value, others by pointer
static HHashValue hash_symbol(const void *p)
{
  const HCFChoice *x=p;
  if(x->type == HCF_END)
    return 0;
  else if(x->type == HCF_CHAR)
    return x->chr * 33;
  else
    return h_hash_ptr(p);
}

// compare LALR items by value
static bool eq_lalr_item(const void *p, const void *q)
{
  const HLRItem *a=p, *b=q;

  if(a->lhs != b->lhs) return false;
  if(a->mark != b->mark) return false;
  if(a->len != b->len) return false;

  for(size_t i=0; i<a->len; i++)
    if(a->rhs[i] != b->rhs[i]) return false;

  return true;
}

// compare LALR item sets (DFA states)
static inline bool eq_lalr_itemset(const void *p, const void *q)
{
  return h_hashset_equal(p, q);
}

// hash LALR items
static inline HHashValue hash_lalr_item(const void *p)
{
  const HLRItem *x = p;
  return (h_hash_ptr(x->lhs)
          + h_djbhash((uint8_t *)x->rhs, x->len*sizeof(HCFChoice *))
          + x->mark);   // XXX is it okay to just add mark?
}

// hash LALR item sets (DFA states) - hash the elements and sum
static HHashValue hash_lalr_itemset(const void *p)
{
  HHashValue hash = 0;

  const HHashTable *ht = p;
  for(size_t i=0; i < ht->capacity; i++) {
    for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;

      hash += hash_lalr_item(hte->key);
    }
  }

  return hash;
}

HLRItem *h_lritem_new(HArena *a, HCFChoice *lhs, HCFChoice **rhs, size_t mark)
{
  HLRItem *ret = h_arena_malloc(a, sizeof(HLRItem));

  size_t len = 0;
  for(HCFChoice **p=rhs; *p; p++) len++;
  assert(mark <= len);

  ret->lhs = lhs;
  ret->rhs = rhs;
  ret->len = len;
  ret->mark = mark;

  return ret;
}

static inline HLRState *h_lrstate_new(HArena *arena)
{
  return h_hashset_new(arena, eq_lalr_item, hash_lalr_item);
}

HLRTable *h_lrtable_new(HAllocator *mm__, size_t nrows)
{
  HArena *arena = h_new_arena(mm__, 0);    // default blocksize
  assert(arena != NULL);

  HLRTable *ret = h_new(HLRTable, 1);
  ret->nrows = nrows;
  ret->rows = h_arena_malloc(arena, nrows * sizeof(HHashTable *));
  ret->forall = h_arena_malloc(arena, nrows * sizeof(HLRAction *));
  ret->inadeq = h_slist_new(arena);
  ret->arena = arena;
  ret->mm__ = mm__;

  for(size_t i=0; i<nrows; i++) {
    ret->rows[i] = h_hashtable_new(arena, eq_symbol, hash_symbol);
    ret->forall[i] = NULL;
  }

  return ret;
}

void h_lrtable_free(HLRTable *table)
{
  HAllocator *mm__ = table->mm__;
  h_delete_arena(table->arena);
  h_free(table);
}

// XXX replace other hashtable iterations with this
// XXX move to internal.h or something
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



/* Constructing the characteristic automaton (handle recognizer) */

static HLRItem *advance_mark(HArena *arena, const HLRItem *item)
{
  assert(item->rhs[item->mark] != NULL);
  HLRItem *ret = h_arena_malloc(arena, sizeof(HLRItem));
  *ret = *item;
  ret->mark++;
  return ret;
}

static HHashSet *closure(HCFGrammar *g, const HHashSet *items)
{
  HAllocator *mm__ = g->mm__;
  HArena *arena = g->arena;
  HHashSet *ret = h_lrstate_new(arena);
  HSlist *work = h_slist_new(arena);

  // iterate over items - initialize work list with them
  const HHashTable *ht = items;
  for(size_t i=0; i < ht->capacity; i++) {
    for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
  
      const HLRItem *item = hte->key;
      h_hashset_put(ret, item);
      h_slist_push(work, (void *)item);
    }
  }

  while(!h_slist_empty(work)) {
    const HLRItem *item = h_slist_pop(work);
    HCFChoice *sym = item->rhs[item->mark]; // symbol after mark

    // if there is a non-terminal after the mark, follow it
    // NB: unlike LLk, we do consider HCF_CHARSET a non-terminal here
    if(sym != NULL && (sym->type==HCF_CHOICE || sym->type==HCF_CHARSET)) {
      // add items corresponding to the productions of sym
      if(sym->type == HCF_CHOICE) {
        for(HCFSequence **p=sym->seq; *p; p++) {
          HLRItem *it = h_lritem_new(arena, sym, (*p)->items, 0);
          if(!h_hashset_present(ret, it)) {
            h_hashset_put(ret, it);
            h_slist_push(work, it);
          }
        }
      } else {  // HCF_CHARSET
        for(unsigned int i=0; i<256; i++) {
          if(charset_isset(sym->charset, i)) {
            // XXX allocatethese single-character symbols statically somewhere
            HCFChoice **rhs = h_new(HCFChoice *, 2);
            rhs[0] = h_new(HCFChoice, 1);
            rhs[0]->type = HCF_CHAR;
            rhs[0]->chr = i;
            rhs[1] = NULL;
            HLRItem *it = h_lritem_new(arena, sym, rhs, 0);
            h_hashset_put(ret, it);
            // single-character item needs no further work
          }
        }
        // if sym is a non-terminal, we need a reshape on it
        // this seems as good a place as any to set it
        sym->reshape = h_act_first;
      }

      // if sym derives epsilon, also advance over it
      if(h_derives_epsilon(g, sym)) {
        HLRItem *it = advance_mark(arena, item);
        h_hashset_put(ret, it);
        h_slist_push(work, it);
      }
    }
  }

  return ret;
}

HLRDFA *h_lr0_dfa(HCFGrammar *g)
{
  HArena *arena = g->arena;

  HHashSet *states = h_hashset_new(arena, eq_lalr_itemset, hash_lalr_itemset);
      // maps itemsets to assigned array indices
  HSlist *transitions = h_slist_new(arena);

  // list of states that need to be processed
  // to save lookups, we push two elements per state, the itemset and its
  // assigned index.
  HSlist *work = h_slist_new(arena);

  // XXX augment grammar?!

  // make initial state (kernel)
  HLRState *start = h_lrstate_new(arena);
  assert(g->start->type == HCF_CHOICE);
  for(HCFSequence **p=g->start->seq; *p; p++)
    h_hashset_put(start, h_lritem_new(arena, g->start, (*p)->items, 0));
  h_hashtable_put(states, start, 0);
  h_slist_push(work, start);
  h_slist_push(work, 0);
  
  // while work to do (on some state)
  //   compute closure
  //   determine edge symbols
  //   for each edge symbol:
  //     advance respective items -> destination state (kernel)
  //     if destination is a new state:
  //       add it to state set
  //       add transition to it
  //       add it to the work list

  while(!h_slist_empty(work)) {
    size_t state_idx = (uintptr_t)h_slist_pop(work);
    HLRState *state = h_slist_pop(work);

    // maps edge symbols to neighbor states (item sets) of s
    HHashTable *neighbors = h_hashtable_new(arena, eq_symbol, hash_symbol);

    // iterate over closure and generate neighboring sets
    const HHashTable *ht = closure(g, state);
    for(size_t i=0; i < ht->capacity; i++) {
      for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
        if(hte->key == NULL)
          continue;

        const HLRItem *item = hte->key;
        HCFChoice *sym = item->rhs[item->mark]; // symbol after mark

        if(sym != NULL) { // mark was not at the end
          // find or create prospective neighbor set
          HLRState *neighbor = h_hashtable_get(neighbors, sym);
          if(neighbor == NULL) {
            neighbor = h_lrstate_new(arena);
            h_hashtable_put(neighbors, sym, neighbor);
          }

          // ...and add the advanced item to it
          h_hashset_put(neighbor, advance_mark(arena, item));
        }
      }
    }

    // merge neighbor sets into the set of existing states
    ht = neighbors;
    for(size_t i=0; i < ht->capacity; i++) {
      for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
        if(hte->key == NULL)
          continue;

        const HCFChoice *symbol = hte->key;
        HLRState *neighbor = hte->value;

        // look up existing state, allocate new if not found
        size_t neighbor_idx;
        if(!h_hashset_present(states, neighbor)) {
          neighbor_idx = states->used;
          h_hashtable_put(states, neighbor, (void *)(uintptr_t)neighbor_idx);
          h_slist_push(work, neighbor);
          h_slist_push(work, (void *)(uintptr_t)neighbor_idx);
        } else {
          neighbor_idx = (uintptr_t)h_hashtable_get(states, neighbor);
        }

        // add transition "state --symbol--> neighbor"
        HLRTransition *t = h_arena_malloc(arena, sizeof(HLRTransition));
        t->from = state_idx;
        t->to = neighbor_idx;
        t->symbol = symbol;
        h_slist_push(transitions, t);
      }
    }
  } // end while(work)

  // fill DFA struct
  HLRDFA *dfa = h_arena_malloc(arena, sizeof(HLRDFA));
  dfa->nstates = states->used;
  dfa->states = h_arena_malloc(arena, dfa->nstates*sizeof(HLRState *));
  for(size_t i=0; i < states->capacity; i++) {
    for(HHashTableEntry *hte = &states->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;

      const HLRState *state = hte->key;
      size_t idx = (uintptr_t)hte->value;

      dfa->states[idx] = state;
    }
  }
  dfa->transitions = transitions;

  return dfa;
}



/* LR(0) table generation */

static HLRAction *shift_action(HArena *arena, size_t nextstate)
{
  HLRAction *action = h_arena_malloc(arena, sizeof(HLRAction));
  action->type = HLR_SHIFT;
  action->nextstate = nextstate;
  return action;
}

static HLRAction *reduce_action(HArena *arena, const HLRItem *item)
{
  HLRAction *action = h_arena_malloc(arena, sizeof(HLRAction));
  action->type = HLR_REDUCE;
  action->production.lhs = item->lhs;
  action->production.length = item->len;
#ifndef NDEBUG
  action->production.rhs = item->rhs;
#endif
  return action;
}

HLRTable *h_lr0_table(HCFGrammar *g, const HLRDFA *dfa)
{
  HAllocator *mm__ = g->mm__;

  HLRTable *table = h_lrtable_new(mm__, dfa->nstates);
  HArena *arena = table->arena;

  // remember start symbol
  table->start = g->start;

  // add shift entries
  for(HSlistNode *x = dfa->transitions->head; x; x = x->next) {
    // for each transition x-A->y, add "shift, goto y" to table entry (x,A)
    HLRTransition *t = x->elem;

    HLRAction *action = shift_action(arena, t->to);
    h_hashtable_put(table->rows[t->from], t->symbol, action);
  }

  // add reduce entries, record inadequate states
  for(size_t i=0; i<dfa->nstates; i++) {
    // find reducible items in state
    H_FOREACH_KEY(dfa->states[i], HLRItem *item)
      if(item->mark == item->len) { // mark at the end
        // check for conflicts
        // XXX store more informative stuff in the inadeq records?
        if(table->forall[i]) {
          // reduce/reduce conflict with a previous item
          h_slist_push(table->inadeq, (void *)(uintptr_t)i);
        } else if(!h_hashtable_empty(table->rows[i])) {
          // shift/reduce conflict with one of the row's entries
          h_slist_push(table->inadeq, (void *)(uintptr_t)i);
        }

        // set reduce action for the entire row
        table->forall[i] = reduce_action(arena, item);
      }
    H_END_FOREACH
  }

  return table;
}



/* LALR-via-SLR grammar transformation */

static inline size_t seqsize(void *p_)
{
  size_t n=0;
  for(void **p=p_; *p; p++) n++;
  return n+1;
}

static size_t follow_transition(const HLRTable *table, size_t x, HCFChoice *A)
{
  HLRAction *action = h_hashtable_get(table->rows[x], A);
  assert(action != NULL);
  assert(action->type == HLR_SHIFT);
  return action->nextstate;
}

static inline HLRTransition *transition(HArena *arena,
                                        size_t x, const HCFChoice *A, size_t y)
{
  HLRTransition *t = h_arena_malloc(arena, sizeof(HLRTransition));
  t->from = x;
  t->symbol = A;
  t->to = y;
  return t;
}

// no-op on terminal symbols
static void transform_productions(const HLRTable *table, HLREnhGrammar *eg,
                                  size_t x, HCFChoice *xAy)
{
  if(xAy->type != HCF_CHOICE)
    return;
  // XXX CHARSET?

  HArena *arena = eg->arena;

  HCFSequence **seq = h_arena_malloc(arena, seqsize(xAy->seq)
                                            * sizeof(HCFSequence *));
  HCFSequence **p, **q;
  for(p=xAy->seq, q=seq; *p; p++, q++) {
    // trace rhs starting in state x and following the transitions
    // xAy -> ... iBj ...

    size_t i = x;
    HCFChoice **B = (*p)->items;
    HCFChoice **items = h_arena_malloc(arena, seqsize(B) * sizeof(HCFChoice *));
    HCFChoice **iBj = items;
    for(; *B; B++, iBj++) {
      size_t j = follow_transition(table, i, *B);
      HLRTransition *i_B_j = transition(arena, i, *B, j);
      *iBj = h_hashtable_get(eg->tmap, i_B_j);
      assert(*iBj != NULL);
      i = j;
    }
    *iBj = NULL;

    *q = h_arena_malloc(arena, sizeof(HCFSequence));
    (*q)->items = items;
  }
  *q = NULL;
  xAy->seq = seq;
}

static bool eq_transition(const void *p, const void *q)
{
  const HLRTransition *a=p, *b=q;
  return (a->from == b->from && a->to == b->to && eq_symbol(a->symbol, b->symbol));
}

static HHashValue hash_transition(const void *p)
{
  const HLRTransition *t = p;
  return (hash_symbol(t->symbol) + t->from + t->to); // XXX ?
}

HCFChoice *new_enhanced_symbol(HLREnhGrammar *eg, const HCFChoice *sym)
{
  HArena *arena = eg->arena;
  HCFChoice *esym = h_arena_malloc(arena, sizeof(HCFChoice));
  *esym = *sym;

  HHashSet *cs = h_hashtable_get(eg->corr, sym);
  if(!cs) {
    cs = h_hashset_new(arena, eq_symbol, hash_symbol);
    h_hashtable_put(eg->corr, sym, cs);
  }
  h_hashset_put(cs, esym);

  return esym;
}
static HLREnhGrammar *enhance_grammar(const HCFGrammar *g, const HLRDFA *dfa,
                                      const HLRTable *table)
{
  HAllocator *mm__ = g->mm__;
  HArena *arena = g->arena;

  HLREnhGrammar *eg = h_arena_malloc(arena, sizeof(HLREnhGrammar));
  eg->tmap = h_hashtable_new(arena, eq_transition, hash_transition);
  eg->smap = h_hashtable_new(arena, eq_symbol, hash_symbol);
  eg->corr = h_hashtable_new(arena, eq_symbol, hash_symbol);
  eg->arena = arena;

  // establish mapping between transitions and symbols
  for(HSlistNode *x=dfa->transitions->head; x; x=x->next) {
    HLRTransition *t = x->elem;

    assert(!h_hashtable_present(eg->tmap, t));

    HCFChoice *sym = new_enhanced_symbol(eg, t->symbol);
    h_hashtable_put(eg->tmap, t, sym);
    h_hashtable_put(eg->smap, sym, t);
  }

  // transform the productions
  H_FOREACH(eg->tmap, HLRTransition *t, HCFChoice *sym)
    transform_productions(table, eg, t->from, sym);
  H_END_FOREACH

  // add the start symbol
  HCFChoice *start = new_enhanced_symbol(eg, g->start);
  transform_productions(table, eg, 0, start);

  eg->grammar = h_cfgrammar_(mm__, start);
  return eg;
}



/* LALR table generation */

static inline bool has_conflicts(HLRTable *table)
{
  return !h_slist_empty(table->inadeq);
}

// place a new entry in tbl; records conflicts in tbl->inadeq
// returns 0 on success, -1 on conflict
// ignores forall entries
int h_lrtable_put(HLRTable *tbl, size_t state, HCFChoice *x, HLRAction *action)
{
  HLRAction *prev = h_hashtable_get(tbl->rows[state], x);
  if(prev && prev != action) {
    // conflict
    h_slist_push(tbl->inadeq, (void *)(uintptr_t)state);
    return -1;
  } else {
    h_hashtable_put(tbl->rows[state], x, action);
    return 0;
  }
}

// check whether a sequence of enhanced-grammar symbols (p) matches the given
// (original-grammar) production rhs and terminates in the given end state.
bool match_production(HLREnhGrammar *eg, HCFChoice **p,
                      HCFChoice **rhs, size_t endstate)
{
  HLRTransition *t;
  for(; *p && *rhs; p++, rhs++) {
    t = h_hashtable_get(eg->smap, *p);
    assert(t != NULL);
    if(!eq_symbol(t->symbol, *rhs))
      return false;
  }
  return (*p == *rhs    // both NULL
          && t->to == endstate);
}

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // generate CFG from parser
  // construct LR(0) DFA
  // build LR(0) table
  // if necessary, resolve conflicts "by conversion to SLR"

  HCFGrammar *g = h_cfgrammar(mm__, parser);
  if(g == NULL)     // backend not suitable (language not context-free)
    return -1;

  HLRDFA *dfa = h_lr0_dfa(g);
  if(dfa == NULL) {     // this should normally not happen
    h_cfgrammar_free(g);
    return -1;
  }

  HLRTable *table = h_lr0_table(g, dfa);
  if(table == NULL) {   // this should normally not happen
    h_cfgrammar_free(g);
    return -1;
  }

  if(has_conflicts(table)) {
    HArena *arena = table->arena;

    HLREnhGrammar *eg = enhance_grammar(g, dfa, table);
    if(eg == NULL) {    // this should normally not happen
      h_cfgrammar_free(g);
      h_lrtable_free(table);
      return -1;
    }

    // go through the inadequate states; replace inadeq with a new list
    HSlist *inadeq = table->inadeq;
    table->inadeq = h_slist_new(arena);
    
    for(HSlistNode *x=inadeq->head; x; x=x->next) {
      size_t state = (uintptr_t)x->elem;
      
      // clear old forall entry, it's being replaced by more fine-grained ones
      table->forall[state] = NULL;

      // go through each reducible item of state
      H_FOREACH_KEY(dfa->states[state], HLRItem *item)
        if(item->mark < item->len)
          continue;

        // action to place in the table cells indicated by lookahead
        HLRAction *action = reduce_action(arena, item);

        // find all LR(0)-enhanced productions matching item
        HHashSet *lhss = h_hashtable_get(eg->corr, item->lhs);
        assert(lhss != NULL);
        H_FOREACH_KEY(lhss, HCFChoice *lhs)
          assert(lhs->type == HCF_CHOICE);  // XXX could be CHARSET?

          for(HCFSequence **p=lhs->seq; *p; p++) {
            HCFChoice **rhs = (*p)->items;
            if(!match_production(eg, rhs, item->rhs, state))
              continue;

            // the left-hand symbol's follow set is this production's
            // contribution to the lookahead
            const HStringMap *fs = h_follow(1, eg->grammar, lhs);
            assert(fs != NULL);

            // for each lookahead symbol, put action into table cell
            if(fs->end_branch) {
              HCFChoice *terminal = h_arena_malloc(arena, sizeof(HCFChoice));
              terminal->type = HCF_END;
              h_lrtable_put(table, state, terminal, action);
            }
            H_FOREACH(fs->char_branches, void *key, HStringMap *m)
              if(!m->epsilon_branch)
                continue;

              HCFChoice *terminal = h_arena_malloc(arena, sizeof(HCFChoice));
              terminal->type = HCF_CHAR; 
              terminal->chr = key_char((HCharKey)key);

              h_lrtable_put(table, state, terminal, action);
            H_END_FOREACH  // lookahead character
        } H_END_FOREACH // enhanced production
      H_END_FOREACH  // reducible item
    }
  }

  h_cfgrammar_free(g);
  parser->backend_data = table;
  return has_conflicts(table)? -1 : 0;
}

void h_lalr_free(HParser *parser)
{
  HLRTable *table = parser->backend_data;
  h_lrtable_free(table);
  parser->backend_data = NULL;
  parser->backend = PB_PACKRAT;
}



/* LR driver */

const HLRAction *
h_lr_lookup(const HLRTable *table, size_t state, const HCFChoice *symbol)
{
  assert(state < table->nrows);
  if(table->forall[state]) {
    assert(h_hashtable_empty(table->rows[state]));  // that would be a conflict
    return table->forall[state];
  } else {
    return h_hashtable_get(table->rows[state], symbol);
  }
}

HParseResult *h_lr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  HLRTable *table = parser->backend_data;
  if(!table)
    return NULL;

  HArena *arena  = h_new_arena(mm__, 0);    // will hold the results
  HArena *tarena = h_new_arena(mm__, 0);    // tmp, deleted after parse
  HSlist *left = h_slist_new(tarena);   // left stack; reductions happen here
  HSlist *right = h_slist_new(tarena);  // right stack; input appears here

  // stack layout:
  // on the left stack, we put pairs:  (saved state, semantic value)
  // on the right stack, we put pairs: (symbol, semantic value)

  // run while the recognizer finds handles in the input
  size_t state = 0;
  while(1) {
    // make sure there is input on the right stack
    if(h_slist_empty(right)) {
      HCFChoice *x = h_arena_malloc(tarena, sizeof(HCFChoice));
      HParsedToken *v;

      uint8_t c = h_read_bits(stream, 8, false);

      if(stream->overrun) {     // end of input
        x->type = HCF_END;
        v = NULL;
      } else {
        x->type = HCF_CHAR;
        x->chr = c;
        v = h_arena_malloc(arena, sizeof(HParsedToken));
        v->token_type = TT_UINT;
        v->uint = c;
      }

      h_slist_push(right, v);
      h_slist_push(right, x);
    }

    // peek at input symbol on the right side
    HCFChoice *symbol = right->head->elem;

    // table lookup
    const HLRAction *action = h_lr_lookup(table, state, symbol);
    if(action == NULL)
      break;    // no handle recognizable in input, terminate parsing

    if(action->type == HLR_SHIFT) {
      h_slist_push(left, (void *)(uintptr_t)state);
      h_slist_pop(right);                       // symbol (discard)
      h_slist_push(left, h_slist_pop(right));   // semantic value
      state = action->nextstate;
    } else {
      assert(action->type == HLR_REDUCE);
      size_t len = action->production.length;
      HCFChoice *symbol = action->production.lhs;

      // semantic value of the reduction result
      HParsedToken *value = h_arena_malloc(arena, sizeof(HParsedToken));
      value->token_type = TT_SEQUENCE;
      value->seq = h_carray_new_sized(arena, len);
      
      // pull values off the left stack, rewinding state accordingly
      HParsedToken *v;
      for(size_t i=0; i<len; i++) {
        v = h_slist_pop(left);
        state = (uintptr_t)h_slist_pop(left);

        // collect values in result sequence
        value->seq->elements[len-1-i] = v;
        value->seq->used++;
      }
      if(v) {
        // result position equals position of left-most symbol
        value->index = v->index;
        value->bit_offset = v->bit_offset;
      } else {
        // XXX how to get the position in this case?
      }

      // perform token reshape if indicated
      if(symbol->reshape)
        value = (HParsedToken *)symbol->reshape(make_result(arena, value));

      // call validation and semantic action, if present
      if(symbol->pred && !symbol->pred(make_result(tarena, value)))
        break;  // validation failed -> no parse
      if(symbol->action)
        value = (HParsedToken *)symbol->action(make_result(arena, value));

      // push result (value, symbol) onto the right stack
      h_slist_push(right, value);
      h_slist_push(right, symbol);
    }
  }



  // parsing was successful iff the start symbol is on top of the right stack
  HParseResult *result = NULL;
  if(h_slist_pop(right) == table->start) {
    // next on the right stack is the start symbol's semantic value
    assert(!h_slist_empty(right));
    HParsedToken *tok = h_slist_pop(right);
    result = make_result(arena, tok);
  } else {
    h_delete_arena(arena);
    result = NULL;
  }

  h_delete_arena(tarena);
  return result;
}



/* Pretty-printers */

void h_pprint_lritem(FILE *f, const HCFGrammar *g, const HLRItem *item)
{
  h_pprint_symbol(f, g, item->lhs);
  fputs(" ->", f);

  HCFChoice **x = item->rhs;
  HCFChoice **mark = item->rhs + item->mark;
  if(*x == NULL) {
    fputs("\"\"", f);
  } else {
    while(*x) {
      if(x == mark)
        fputc('.', f);
      else
        fputc(' ', f);

      if((*x)->type == HCF_CHAR) {
        // condense character strings
        fputc('"', f);
        h_pprint_char(f, (*x)->chr);
        for(x++; *x; x++) {
          if(x == mark)
            break;
          if((*x)->type != HCF_CHAR)
            break;
          h_pprint_char(f, (*x)->chr);
        }
        fputc('"', f);
      } else {
        h_pprint_symbol(f, g, *x);
        x++;
      }
    }
    if(x == mark)
      fputs(".", f);
  }
}

void h_pprint_lrstate(FILE *f, const HCFGrammar *g,
                      const HLRState *state, unsigned int indent)
{
  bool first = true;
  const HHashTable *ht = state;
  for(size_t i=0; i < ht->capacity; i++) {
    for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
  
      const HLRItem *item = hte->key;

      if(!first)
        for(unsigned int i=0; i<indent; i++) fputc(' ', f);
      first = false;
      h_pprint_lritem(f, g, item);
      fputc('\n', f);
    }
  }
}

static void pprint_transition(FILE *f, const HCFGrammar *g, const HLRTransition *t)
{
  fputs("-", f);
  h_pprint_symbol(f, g, t->symbol);
  fprintf(f, "->%lu", t->to);
}

void h_pprint_lrdfa(FILE *f, const HCFGrammar *g,
                    const HLRDFA *dfa, unsigned int indent)
{
  for(size_t i=0; i<dfa->nstates; i++) {
    unsigned int indent2 = indent + fprintf(f, "%4lu: ", i);
    h_pprint_lrstate(f, g, dfa->states[i], indent2);
    for(HSlistNode *x = dfa->transitions->head; x; x = x->next) {
      const HLRTransition *t = x->elem;
      if(t->from == i) {
        for(unsigned int i=0; i<indent2-2; i++) fputc(' ', f);
        pprint_transition(f, g, t);
        fputc('\n', f);
      }
    }
  }
}

void pprint_lraction(FILE *f, const HCFGrammar *g, const HLRAction *action)
{
  if(action->type == HLR_SHIFT) {
    fprintf(f, "s%lu", action->nextstate);
  } else {
    fputs("r(", f);
    h_pprint_symbol(f, g, action->production.lhs);
    fputs(" -> ", f);
#ifdef NDEBUG
    // if we can't print the production, at least print its length
    fprintf(f, "[%lu]", action->production.length);
#else
    HCFSequence seq = {action->production.rhs};
    h_pprint_sequence(f, g, &seq);
#endif
    fputc(')', f);
  }
}

void h_pprint_lrtable(FILE *f, const HCFGrammar *g, const HLRTable *table,
                      unsigned int indent)
{
  for(size_t i=0; i<table->nrows; i++) {
    for(unsigned int j=0; j<indent; j++) fputc(' ', f);
    fprintf(f, "%4lu:", i);
    if(table->forall[i]) {
      fputs(" - ", f);
      pprint_lraction(f, g, table->forall[i]);
      fputs(" -", f);
      if(!h_hashtable_empty(table->rows[i]))
        fputs(" !!", f);
    }
    H_FOREACH(table->rows[i], HCFChoice *symbol, HLRAction *action)
      fputc(' ', f);    // separator
      h_pprint_symbol(f, g, symbol);
      fputc(':', f);
      if(table->forall[i]) {
        fputc(action->type == HLR_SHIFT? 's' : 'r', f);
        fputc('/', f);
        fputc(table->forall[i]->type == HLR_SHIFT? 's' : 'r', f);
      } else {
        pprint_lraction(f, g, action);
      }
    H_END_FOREACH
    fputc('\n', f);
  }

#if 0
  fputs("inadeq=", f);
  for(HSlistNode *x=table->inadeq->head; x; x=x->next) {
    fprintf(f, "%lu ", (uintptr_t)x->elem);
  }
  fputc('\n', f);
#endif
}



HParserBackendVTable h__lalr_backend_vtable = {
  .compile = h_lalr_compile,
  .parse = h_lr_parse,
  .free = h_lalr_free
};




// dummy!
int test_lalr(void)
{
  /* 
     S -> E
     E -> E '-' T
        | T
     T -> '(' E ')'
        | 'n'               -- also try [0-9] for the charset paths
  */

  HParser *n = h_ch('n');
  HParser *E = h_indirect();
  HParser *T = h_choice(h_sequence(h_ch('('), E, h_ch(')'), NULL), n, NULL);
  HParser *E_ = h_choice(h_sequence(E, h_ch('-'), T, NULL), T, NULL);
  h_bind_indirect(E, E_);
  HParser *p = h_sequence(E, NULL);

  printf("\n==== G R A M M A R ====\n");
  HCFGrammar *g = h_cfgrammar(&system_allocator, p);
  if(g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }
  h_pprint_grammar(stdout, g, 0);

  printf("\n==== D F A ====\n");
  HLRDFA *dfa = h_lr0_dfa(g);
  if(dfa)
    h_pprint_lrdfa(stdout, g, dfa, 0);
  else
    fprintf(stderr, "h_lalr_dfa failed\n");

  printf("\n==== L R ( 0 )  T A B L E ====\n");
  HLRTable *table0 = h_lr0_table(g, dfa);
  if(table0)
    h_pprint_lrtable(stdout, g, table0, 0);
  else
    fprintf(stderr, "h_lr0_table failed\n");
  h_lrtable_free(table0);

  printf("\n==== L A L R  T A B L E ====\n");
  if(h_compile(p, PB_LALR, NULL)) {
    fprintf(stderr, "does not compile\n");
    return 2;
  }
  h_pprint_lrtable(stdout, g, (HLRTable *)p->backend_data, 0);

  printf("\n==== P A R S E  R E S U L T ====\n");
  HParseResult *res = h_parse(p, (uint8_t *)"n-(n-((n)))-n", 13);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
