#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"



/* Data structures */

//  - states are hashsets containing LRItems
//  - LRItems contain an optional lookahead set (HStringMap)
//  - states (hashsets) get hash and comparison functions that ignore the lookahead

typedef HHashSet HLRState;

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
  HStringMap *lookahead;    // optional
} HLRItem;

typedef struct HLRAction_ {
  enum {HLR_SHIFT, HLR_REDUCE} type;
  union {
    size_t nextstate;   // used with SHIFT
    struct {
      HCFChoice *lhs;   // symbol carrying semantic actions etc.
      size_t length;    // # of symbols in rhs
                        // NB: the rhs symbols are not needed for the parse
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



/* Constructing the characteristic automaton (handle recognizer) */

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
  ret->lookahead = NULL;

  return ret;
}

// compare LALR items - ignores lookahead
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

static inline HLRState *h_lrstate_new(HArena *arena)
{
  return h_hashset_new(arena, eq_lalr_item, hash_lalr_item);
}

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
    // XXX: do we have to count HCF_CHARSET as nonterminal?
    if(sym != NULL && sym->type == HCF_CHOICE) {
      // add items corresponding to the productions of sym
      for(HCFSequence **p=sym->seq; *p; p++) {
        HLRItem *it = h_lritem_new(arena, sym, (*p)->items, 0);
        if(!h_hashset_present(ret, it)) {
          h_hashset_put(ret, it);
          h_slist_push(work, it);
        }
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
    HHashTable *neighbors = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);

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
    ret->rows[i] = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);
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

static HLRAction *shift_action(HArena *arena, size_t nextstate)
{
  HLRAction *action = h_arena_malloc(arena, sizeof(HLRAction));
  action->type = HLR_SHIFT;
  action->nextstate = nextstate;
  return action;
}

static HLRAction *reduce_action(HArena *arena, HCFChoice *lhs, size_t rhslen)
{
  HLRAction *action = h_arena_malloc(arena, sizeof(HLRAction));
  action->type = HLR_REDUCE;
  action->production.lhs = lhs;
  action->production.length = rhslen;
  return action;
}

HLRTable *h_lr0_table(HCFGrammar *g)
{
  HAllocator *mm__ = g->mm__;

  // construct LR(0) DFA
  HLRDFA *dfa = h_lr0_dfa(g);
  if(!dfa) return NULL;

  HLRTable *table = h_lrtable_new(mm__, dfa->nstates);
  HArena *arena = table->arena;

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
        // XXX store more informative stuff in the inadeq records?
        if(table->forall[i]) {
          // reduce/reduce conflict with a previous item
          h_slist_push(table->inadeq, (void *)(uintptr_t)i);
        } else if(!h_hashtable_empty(table->rows[i])) {
          // shift/reduce conflict with one of the row's entries
          h_slist_push(table->inadeq, (void *)(uintptr_t)i);
        } else {
          // set reduce action for the entire row
          table->forall[i] = reduce_action(arena, item->lhs, item->len);
        }
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

static HCFChoice *transform_symbol(const HLRTable *table, HHashTable *map,
                                   size_t x, HCFChoice *B, size_t z);

static HCFChoice *transform_productions(const HLRTable *table, HHashTable *map,
                                         size_t x, HCFChoice *xAy)
{
  HArena *arena = map->arena;

  HCFSequence **seq = h_arena_malloc(arena, seqsize(xAy->seq)
                                            * sizeof(HCFSequence *));
  HCFSequence **p, **q;
  for(p=xAy->seq, q=seq; *p; p++, q++) {
    // trace rhs starting in state x and following the transitions
    // xAy -> xBz ...

    HCFChoice **B = (*p)->items;
    HCFChoice **xBz = h_arena_malloc(arena, seqsize(B) * sizeof(HCFChoice *));
    for(; *B; B++, xBz++) {
      size_t z = follow_transition(table, x, *B);
      *xBz = transform_symbol(table, map, x, *B, z);
      x=z;
    }
    *xBz = NULL;

    *q = h_arena_malloc(arena, sizeof(HCFSequence));
    (*q)->items = xBz;
  }
  *q = NULL;
  xAy->seq = seq;

  return xAy;   // pass-through
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

static HCFChoice *transform_symbol(const HLRTable *table, HHashTable *map,
                                   size_t x, HCFChoice *B, size_t z)
{
  HArena *arena = map->arena;

  // look up the transition in map, create symbol if not found
  HLRTransition *x_B_z = transition(arena, x, B, z);
  HCFChoice *xBz = h_hashtable_get(map, x_B_z);
  if(!xBz) {
    HCFChoice *xBz = h_arena_malloc(arena, sizeof(HCFChoice));
    *xBz = *B;
    h_hashtable_put(map, x_B_z, xBz);
  }

  return transform_productions(table, map, x, xBz);
}

static bool eq_transition(const void *p, const void *q)
{
  const HLRTransition *a=p, *b=q;
  return (a->from == b->from && a->to == b->to && a->symbol == b->symbol);
}

static HHashValue hash_transition(const void *p)
{
  const HLRTransition *t = p;
  return (h_hash_ptr(t->symbol) + t->from + t->to); // XXX ?
}

static HHashTable *enhance_grammar(const HCFGrammar *g, const HLRTable *tbl)
{
  HArena *arena = g->arena; // XXX ?
  HHashTable *map = h_hashtable_new(arena, eq_transition, hash_transition);

  // copy the start symbol over
  HCFChoice *start = h_arena_malloc(arena, sizeof(HCFChoice));
  *start = *(g->start);
  h_hashtable_put(map, g->start, start);

  transform_productions(tbl, map, 0, start);

  return map;
}



/* LALR table generation */

bool is_inadequate(HLRTable *table, size_t state)
{
  // XXX
  return false;
}

bool has_conflicts(HLRTable *table)
{
  return !h_slist_empty(table->inadeq);
}

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // generate CFG from parser
  // build LR(0) table
  // if necessary, resolve conflicts "by conversion to SLR"

  HCFGrammar *g = h_cfgrammar(mm__, parser);
  if(g == NULL)     // backend not suitable (language not context-free)
    return -1;

  HLRTable *table = h_lr0_table(g);
  if(table == NULL) // this should normally not happen
    return -1;

  if(has_conflicts(table)) {
    HHashTable *map = enhance_grammar(g, table);
    if(map == NULL) // this should normally not happen
      return -1;

    // XXX resolve conflicts
    // iterate over dfa's transitions where 'from' state is inadequate
    //   look up enhanced symbol corr. to the transition
    //   for each terminal in follow set of enh. symbol:
    //     put reduce action into table cell (state, terminal)
    //     conflict if already occupied
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

HParseResult *h_lr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  return NULL;
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

void pprint_transition(FILE *f, const HCFGrammar *g, const HLRTransition *t)
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
    fputc('r', f);
    // XXX reference the production somehow
  }
}

void h_pprint_lrtable(FILE *f, const HCFGrammar *g, const HLRTable *table,
                      unsigned int indent)
{
  for(size_t i=0; i<table->nrows; i++) {
    for(unsigned int j=0; j<indent; j++) fputc(' ', f);
    fprintf(f, "%4lu:", i);
    if(table->forall[i] && h_hashtable_empty(table->rows[i])) {
      fputs(" - ", f);
      pprint_lraction(f, g, table->forall[i]);
      fputs(" -", f);
    } else {
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
    }
    fputc('\n', f);
  }
}



HParserBackendVTable h__lalr_backend_vtable = {
  .compile = h_lalr_compile,
  .parse = h_lr_parse,
  .free = h_lalr_free
};




// dummy!
int test_lalr(void)
{
  /* for k=2:

     S -> A | B
     A -> X Y a
     B -> Y b
     X -> x | ''
     Y -> y         -- for k=3 use "yy"
  */

  // XXX make LALR example
  HParser *X = h_optional(h_ch('x'));
  HParser *Y = h_sequence(h_ch('y'), h_ch('y'), NULL);
  HParser *A = h_sequence(X, Y, h_ch('a'), NULL);
  HParser *B = h_sequence(Y, h_ch('b'), NULL);
  HParser *p = h_choice(A, B, NULL);

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
  HLRTable *table0 = h_lr0_table(g);
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
  HParseResult *res = h_parse(p, (uint8_t *)"xyya", 4);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
