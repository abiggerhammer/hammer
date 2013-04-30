#include <assert.h>
#include "../internal.h"
#include "../parsers/parser_internal.h"



/* Grammar representation and analysis */

typedef struct HCFGrammar_ {
  HHashSet    *nts;     // HCFChoices, each representing the alternative
                        // productions for one nonterminal
  HHashSet    *geneps;  // set of NTs that can generate the empty string
  HHashTable  *first;   // memoized "first" sets of the grammar's symbols
  HArena      *arena;
} HCFGrammar;

typedef int HCFTerminal;
static HCFTerminal end_token = -1;

bool h_eq_ptr(const void *p, const void *q) { return (p==q); }
HHashValue h_hash_ptr(const void *p) { return (uintptr_t)p; }

HCFGrammar *h_grammar_new(HAllocator *mm__)
{
  HCFGrammar *g = h_new(HCFGrammar, 1);
  assert(g != NULL);

  g->arena  = h_new_arena(mm__, 0);     // default blocksize
  g->nts    = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  g->geneps = NULL;
  g->first  = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);

  return g;
}


// helper
static void collect_nts(HCFGrammar *grammar, HCFChoice *symbol);

/* Convert 'parser' into CFG representation by desugaring and compiling the set
 * of nonterminals.
 * A NULL return means we are unable to represent the parser as a CFG.
 */
HCFGrammar *h_grammar(HAllocator* mm__, const HParser *parser)
{
  // convert parser to CFG form ("desugar").
  HCFChoice *desugared = h_desugar(mm__, parser);
  if(desugared == NULL)
    return NULL;  // -> backend not suitable for this parser

  HCFGrammar *g = h_grammar_new(mm__);

  // recursively traverse the desugared form and collect all HCFChoices that
  // represent a nonterminal (type HCF_CHOICE or HCF_CHARSET).
  collect_nts(g, desugared);
  if(h_hashset_empty(g->nts)) {
    // desugared is a single terminal. wrap it in a singleton HCF_CHOICE.
    HCFChoice *nt = h_new(HCFChoice, 1);
    nt->type = HCF_CHOICE;
    nt->seq = h_new(HCFSequence *, 2);
    nt->seq[0] = h_new(HCFSequence, 1);
    nt->seq[0]->items = h_new(HCFChoice *, 2);
    nt->seq[0]->items[0] = desugared;
    nt->seq[0]->items[1] = NULL;
    nt->seq[1] = NULL;
    h_hashset_put(g->nts, nt);
  }

  // XXX call collect_geneps here?

  return g;
}

/* Add all nonterminals reachable from symbol to grammar. */
static void collect_nts(HCFGrammar *grammar, HCFChoice *symbol)
{
  HCFSequence **s;  // for the rhs (sentential form) of a production
  HCFChoice **x;    // for a symbol in s

  if(h_hashset_present(grammar->nts, symbol))
    return;     // already visited, get out

  switch(symbol->type) {
  case HCF_CHAR:
  case HCF_END:
    break;  // it's a terminal symbol, nothing to do

  case HCF_CHARSET:
    h_hashset_put(grammar->nts, symbol);
    break;  // this type has only terminal children

  case HCF_CHOICE:
    h_hashset_put(grammar->nts, symbol);
    // each element s of symbol->seq (HCFSequence) represents the RHS of
    // a production. call self on all symbols (HCFChoice) in s.
    for(s = symbol->seq; *s != NULL; s++) {
      for(x = (*s)->items; *x != NULL; x++) {
        collect_nts(grammar, *x);
      }
    }
    break;

  default:  // should not be reachable
    assert_message(0, "unknown HCFChoice type");
  }
}


// helper
static void collect_geneps(HCFGrammar *grammar);

/* Does the given symbol derive the empty string (under g)? */
bool h_symbol_derives_epsilon(HCFGrammar *g, const HCFChoice *symbol)
{
  if(g->geneps == NULL)
    collect_geneps(g);
  assert(g->geneps != NULL);

  switch(symbol->type) {
  case HCF_END:       // the end token doesn't count as empty
  case HCF_CHAR:
  case HCF_CHARSET:
    return false;
  default:  // HCF_CHOICE
    return h_hashset_present(g->geneps, symbol);
  }
}

/* Does the sentential form given by s derive the empty string? */
bool h_sequence_derives_epsilon(HCFGrammar *g, const HCFSequence *s)
{
  // return true iff all symbols in s derive epsilon
  HCFChoice **x;
  for(x = s->items; *x; x++) {
    if(!h_symbol_derives_epsilon(g, *x))
      return false;
  }
  return true;
}

/* Populate the geneps member of g; no-op if called multiple times. */
static void collect_geneps(HCFGrammar *g)
{
  if(g->geneps == NULL)
    return;

  g->geneps = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  assert(g->geneps != NULL);

  // iterate over the grammar's symbols, the elements of g->nts.
  // add any we can identify as deriving epsilon to g->geneps.
  // repeat until g->geneps no longer changes.
  size_t prevused = g->nts->used;
  do {
    size_t i;
    HHashTableEntry *hte;
    for(i=0; i < g->nts->capacity; i++) {
      for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
        const HCFChoice *symbol = hte->key;

        // only "choice" nonterminals can derive epsilon.
        if(symbol->type != HCF_CHOICE)
          continue;

        // this NT derives epsilon if any of its productions does.
        HCFSequence **p;
        for(p = symbol->seq; *p != NULL; p++) {
          if(h_sequence_derives_epsilon(g, *p)) {
            h_hashset_put(g->nts, symbol);
            break;
          }
        }
      }
    }
  } while(g->nts->used != prevused);
}


// helper
static HHashSet *first_sequence_(HCFGrammar *g, const HCFChoice **s);

static inline HHashSet *h_first_sequence(HCFGrammar *g, const HCFSequence *s)
{
  // why do I have to cast to a type that's _more_ const?
  return first_sequence_(g, (const HCFChoice **)s->items);
}

static HHashSet *h_first_symbol(HCFGrammar *g, const HCFChoice *x)
{
  HHashSet *ret;
  HCFSequence **p;
  uint8_t c;

  // memoize via g->first
  assert(g->first != NULL);
  ret = h_hashtable_get(g->first, x);
  if(ret != NULL)
    return ret;
  ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  assert(ret != NULL);
  h_hashtable_put(g->first, x, ret);

  switch(x->type) {
  case HCF_END:
    h_hashset_put(ret, (void *)(intptr_t)end_token);
    break;
  case HCF_CHAR:
    h_hashset_put(ret, (void *)(intptr_t)x->chr);
    break;
  case HCF_CHARSET:
    c=0;
    do {
      if(charset_isset(x->charset, c))
        h_hashset_put(ret, (void *)(intptr_t)c);
    } while(c++ < 255);
    break;
  case HCF_CHOICE:
    // this is a nonterminal
    // return the union of the first sets of all productions
    for(p=x->seq; *p; ++p)
      h_hashset_put_all(ret, h_first_sequence(g, *p));
    break;
  default:  // should not be reached
    assert_message(0, "unknown HCFChoice type");
  }
  
  return ret;
}

static HHashSet *first_sequence_(HCFGrammar *g, const HCFChoice **s)
{
  // the first set of the empty sequence is empty
  if(*s == NULL)
    return h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);

  // first(X tail) = first(X)                if X does not derive epsilon
  //               = first(X) u first(tail)  otherwise

  const HCFChoice *x = s[0];
  const HCFChoice **tail = s+1;

  HHashSet *first_x = h_first_symbol(g, x);
  if(h_symbol_derives_epsilon(g, x)) {
    // return the union of first(x) and first(tail)
    HHashSet *first_tail = first_sequence_(g, tail);
    HHashSet *ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
    h_hashset_put_all(ret, first_x);
    h_hashset_put_all(ret, first_tail);
    return ret;
  } else {
    return first_x;
  }
}



/* LL parse table and associated data */

typedef struct HLLTable_ {
  unsigned int **arr;             // Nonterminals numbered from 1, 0 = error.
} HLLTable;

typedef struct HLLData_ {
  HCFGrammar *grammar;
  HLLTable *table;
} HLLData;

#if 0
/* Interface to look up an entry in the parse table. */
unsigned int h_ll_lookup(const HLLTable *table, unsigned int nonterminal, uint8_t token)
{
  assert(nonterminal > 0);
  return table->arr[n*257+token];
}
#endif

// XXX predict_set

int h_ll_compile(HAllocator* mm__, const HParser* parser, const void* params)
{
  // Convert parser to a CFG. This can fail as indicated by a NULL return.
  HCFGrammar *grammar = h_grammar(mm__, parser);
  if(grammar == NULL)
    return -1;                  // -> Backend unsuitable for this parser.

  // TODO: eliminate common prefixes
  // TODO: eliminate left recursion
  // TODO: avoid conflicts by splitting occurances?

  // XXX generate table and store in parser->data.
  // XXX any other data structures needed?

  return -1; // XXX 0 on success
}



/* LL driver */

HParseResult *h_ll_parse(HAllocator* mm__, const HParser* parser, HParseState* parse_state)
{
  // get table from parser->data.
  // run driver.
  return NULL; // TODO
}



HParserBackendVTable h__ll_backend_vtable = {
  .compile = h_ll_compile,
  .parse = h_ll_parse
};
