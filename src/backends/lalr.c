#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"


/* Constructing the characteristic automaton (handle recognizer) */

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
  size_t from, to;      // indices into 'states' array
  const HCFChoice *symbol;
} HLRTransition;

typedef struct HLRItem_ {
  HCFChoice *lhs;
  HCFChoice **rhs;          // NULL-terminated
  size_t len;               // number of elements in rhs
  size_t mark;
  HStringMap *lookahead;    // optional
} HLRItem;

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

HLRDFA *h_lalr_dfa(HCFGrammar *g)
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



/* LALR table generation */

typedef struct HLRAction_ {
  enum {HLR_SHIFT, HLR_REDUCE} type;
  union {
    size_t nextstate;   // used with shift
    struct {
      HCFChoice *lhs;
      HCFChoice **rhs;
    } production;       // used with reduce
  };
} HLRAction;

typedef struct HLRTable_ {
  size_t     nrows;
  HHashTable **rows;    // map symbols to HLRActions
  HCFChoice  *start;    // start symbol
  HArena     *arena;
  HAllocator *mm__;
} HLRTable;

HLRTable *h_lrtable_new(HAllocator *mm__, size_t nrows)
{
  HArena *arena = h_new_arena(mm__, 0);    // default blocksize
  assert(arena != NULL);

  HLRTable *ret = h_new(HLRTable, 1);
  ret->nrows = nrows;
  ret->rows = h_arena_malloc(arena, nrows * sizeof(HHashTable *));
  ret->arena = arena;
  ret->mm__ = mm__;

  for(size_t i=0; i<nrows; i++)
    ret->rows[i] = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);

  return ret;
}

static HCFGrammar *transform_grammar(const HCFGrammar *g, const HLRTable *table,
                                     const HLRDFA *dfa, HHashTable **syms)
{
  HCFGrammar *gt = h_cfgrammar_new(g->mm__);
  HArena *arena = gt->arena;

  // old grammar symbol -> 
  //HHashTable *map = h_hashtable_new(

  for(size_t i=0; i<dfa->nstates; i++) {
    const HLRState *state = dfa->states[i];

    syms[i] = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);
    
    
  }

  // iterate over g->nts
  const HHashTable *ht = g->nts;
  for(size_t i=0; i < ht->capacity; i++) {
    for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;

      const HCFChoice *A = hte->key;

      // iterate over the productions of A
      for(HCFSequence **p=A->seq; *p; p++) {
        // find all transitions marked by A
        // yields xAy -> rhs'
        // trace rhs starting in state x and following the transitions
      }
    }
  }

  return gt;
}

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // generate CFG from parser
  // construct LR(0) DFA
  // build parse table, shift-entries only
  //   for each transition a--S-->b, add "shift, goto b" to table entry (a,S)
  // determine lookahead "by conversion to SLR"
  //   transform grammar to encode transitions in symbols
  //   -> lookahead for an item is the transformed left-hand side's follow set
  // finish table; for each state:
  //   add reduce entries for its accepting items
  //   in case of conflict, add lookahead info

  HCFGrammar *g = h_cfgrammar(mm__, parser);
  if(g == NULL)     // backend not suitable (language not context-free)
    return -1;

  HLRDFA *dfa = h_lalr_dfa(g);
  if(dfa == NULL)   // this should actually not happen
    return -1;

  // create table with shift actions
  HLRTable *table = h_lrtable_new(mm__, dfa->nstates);
  for(HSlistNode *x = dfa->transitions->head; x; x = x->next) {
    HLRTransition *t = x->elem;
    HLRAction *action = h_arena_malloc(table->arena, sizeof(HLRAction));
    action->type = HLR_SHIFT;
    action->nextstate = t->to;
    h_hashtable_put(table->rows[t->from], t->symbol, action);
  }

  // mapping (state,item)-pairs to the symbols of the new grammar
  HHashTable **syms = h_arena_malloc(g->arena, dfa->nstates * sizeof(HHashTable *));
      // XXX use a different arena for this (and other things)

  HCFGrammar *gt = transform_grammar(g, table, dfa, syms);
  if(gt == NULL)   // this should actually not happen
    return -1;

  // XXX fill in reduce actions

  return 0;
}

void h_lalr_free(HParser *parser)
{
  // XXX free data structures
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
  HLRDFA *dfa = h_lalr_dfa(g);
  if(dfa)
    h_pprint_lrdfa(stdout, g, dfa, 0);
  else
    fprintf(stderr, "h_lalr_dfa failed\n");

  printf("\n==== L A L R  T A B L E ====\n");
  if(h_compile(p, PB_LALR, NULL)) {
    fprintf(stderr, "does not compile\n");
    return 2;
  }
  // print LALR(1) table

  printf("\n==== P A R S E  R E S U L T ====\n");
  HParseResult *res = h_parse(p, (uint8_t *)"xyya", 4);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
