#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"



/* LL parse table and associated data */

/* Maps each nonterminal (HCFChoice) of the grammar to another hash table that
 * maps lookahead tokens (HCFToken) to productions (HCFSequence).
 */
typedef HHashTable HLLTable;

/* Interface to look up an entry in the parse table. */
const HCFSequence *h_ll_lookup(const HLLTable *table, const HCFChoice *x, HCFToken tok)
{
  const HHashTable *row = h_hashtable_get(table, x);
  assert(row != NULL);  // the table should have one row for each nonterminal

  const HCFSequence *production = h_hashtable_get(row, (void *)tok);
  return production;
}

/* Compute the predict set of production "A -> rhs". */
HHashSet *h_predict(HCFGrammar *g, const HCFChoice *A, const HCFSequence *rhs)
{
  // predict(A -> rhs) = first(rhs) u follow(A)  if "" can be derived from rhs
  // predict(A -> rhs) = first(rhs)              otherwise
  HHashSet *first_rhs = h_first_sequence(g, rhs->items);
  if(h_sequence_derives_epsilon(g, rhs->items)) {
    HHashSet *ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
    h_hashset_put_all(ret, first_rhs);
    h_hashset_put_all(ret, h_follow(g, A));
    return ret;
  } else {
    return first_rhs;
  }
}

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
  // XXX free grammar and its arena

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
