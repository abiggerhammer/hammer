#include <assert.h>
#include "../internal.h"
#include "../parsers/parser_internal.h"

#define H_LL_END_TOKEN 256


/* Grammar representation and analysis */

typedef struct HCFGrammar_ {
  HHashSet    *nts;     // HCFChoices, each representing the alternative
                        // productions for one nonterminal
  HHashSet    *geneps;  // set of NTs that can generate the empty string
  HHashTable  *first;   // "first" sets of the grammar's nonterminals
  HArena      *arena;
} HCFGrammar;

bool h_eq_pointer(const void *p, const void *q) { return (p==q); }
HHashValue h_hash_ptr(const void *p) { return (uintptr_t)p; }

HCFGrammar *h_grammar_new(HAllocator *mm__)
{
  HCFGrammar *g = h_new(HCFGrammar, 1);
  assert(g != NULL);

  g->arena  = h_new_arena(mm__, 0);     // default blocksize
  g->nts    = h_hashset_new(g->arena, h_eq_pointer, h_hash_ptr);
  g->geneps = NULL;
  g->first  = NULL;

  return g;
}

void h_grammar_collect(HCFGrammar *grammar, HCFChoice *symbol); // helper

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
  // represent a nonterminal.
  h_grammar_collect(g, desugared);

  if(h_hashset_empty(g->nts)) {
    // desugared is a single nonterminal. wrap it.
    // XXX is this even necessary?
  }

  return g;
}

/* Add all nonterminals reachable from symbol to grammar. */
void h_grammar_collect(HCFGrammar *grammar, HCFChoice *symbol)
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
        h_grammar_collect(grammar, *x);
      }
    }
    break;

  default:
    assert_message(0, "unknown HCFChoice type");
  }
}

// first_set(grammar?, production)
// follow_set



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
