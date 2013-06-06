#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"


// PLAN:
// data structures:
//  - LR table is an array of hashtables that map grammar symbols (HCFChoice)
//    to LRActions.

// build LR(0) DFA
// extend with lookahead information by either:
//  - reworking algorithm to propagate lookahead ("simple LALR generation")
//  - follow sets of enhanced grammar ("conversion to SLR")


/* Constructing the characteristic automaton (handle recognizer) */

//  - DFA is a hashset containing states (mapped to numbers)
//  - states are hashsets containing LRItems
//  - LRItems contain an optional lookahead set (HStringMap)
//  - states (hashsets) get hash and comparison functions that ignore the lookahead

typedef struct HLRDFA_ {
  HHashSet *states;
  HSlist *transitions;
} HLRDFA;

typedef struct HLRTransition_ {
  HLRState *from;
  HCFChoice *symbol;
  HLRState *to;
} HLRTransition;

typedef struct HLRItem_ {
  HCFChoice *lhs;
  HCFChoice **rhs;
  size_t len;               // number of elements in rhs
  size_t mark;
  HStringMap *lookahead;    // optional
} HLRItem;

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
static inline HHashValue hash_lalr_item(const HLRItem *x)
{
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

static HHashSet *closure(const HHashSet *items);

HLRDFA *h_lalr_dfa(HCFGrammar *g)
{
  HHashSet *states = h_hashset_new(g->arena, eq_lalr_itemset, hash_lalr_itemset);

  // make initial state (kernel)
  
  // while work to do (on some state)
  //   compute closure
  //   determine edge symbols
  //   for each edge symbol:
  //     advance respective items -> destination state (kernel)
  //     if destination is a new state:
  //       add it to state set
  //       add transition to it
  //       add it to the work list
}



/* LALR table generation */

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // generate grammar
  // construct dfa / determine lookahead
  // extract table
  //   create an array of hashtables, one per state
  //   for each transition a--S-->b:
  //     add "shift, goto b" to table entry (a,S)
  //   for each state:
  //     add reduce entries for its accepting items
  return -1;
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

  HCFGrammar *g = h_cfgrammar(&system_allocator, p);

  if(g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }

  h_pprint_grammar(stdout, g, 0);
  // print states of the LR(0) automaton
  // print LALR(1) table

  if(h_compile(p, PB_LALR, NULL)) {
    fprintf(stderr, "does not compile\n");
    return 2;
  }


  HParseResult *res = h_parse(p, (uint8_t *)"xyya", 4);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
