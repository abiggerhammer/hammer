#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"



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
  HCFGrammar *grammar = h_cfgrammar(mm__, parser);
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




// dummy!
int test_ll(void)
{
  const HParser *c = h_many(h_ch('x'));
  const HParser *q = h_sequence(c, h_ch('y'), NULL);
  const HParser *p = h_choice(q, h_end_p(), NULL);

  HCFGrammar *g = h_cfgrammar(&system_allocator, p);

  if(g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }

  h_pprint_grammar(stdout, g, 0);
  printf("generate epsilon: ");
  h_pprint_symbolset(stdout, g, g->geneps, 0);
  printf("first(A) = ");
  h_pprint_tokenset(stdout, g, h_first_symbol(g, g->start), 0);
  printf("follow(C) = ");
  h_pprint_tokenset(stdout, g, h_follow(g, h_desugar(&system_allocator, c)), 0);

  return 0;
}
