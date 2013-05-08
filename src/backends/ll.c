#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"



/* Generating the LL parse table */

/* Maps each nonterminal (HCFChoice) of the grammar to another hash table that
 * maps lookahead tokens (HCFToken) to productions (HCFSequence).
 */
typedef struct HLLTable_ {
  HHashTable *rows;
  HArena     *arena;
  HAllocator *mm__;
} HLLTable;

/* Interface to look up an entry in the parse table. */
const HCFSequence *h_ll_lookup(const HLLTable *table, const HCFChoice *x, HCFToken tok)
{
  const HHashTable *row = h_hashtable_get(table->rows, x);
  assert(row != NULL);  // the table should have one row for each nonterminal

  const HCFSequence *production = h_hashtable_get(row, (void *)tok);
  return production;
}

/* Allocate a new parse table. */
HLLTable *h_lltable_new(HAllocator *mm__)
{
  // NB the parse table gets an arena separate from the grammar so we can free
  //    the latter after table generation.
  HArena *arena = h_new_arena(mm__, 0);    // default blocksize
  assert(arena != NULL);
  HHashTable *rows = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);
  assert(rows != NULL);

  HLLTable *table = h_new(HLLTable, 1);
  assert(table != NULL);
  table->mm__  = mm__;
  table->arena = arena;
  table->rows  = rows;

  return table;
}

void h_lltable_free(HLLTable *table)
{
  HAllocator *mm__ = table->mm__;
  h_delete_arena(table->arena);
  h_free(table);
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

/* Generate the LL parse table from the given grammar.
 * Returns -1 on error, 0 on success.
 */
static int fill_table(HCFGrammar *g, HLLTable *table)
{
  return -1; // XXX
}

int h_ll_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // Convert parser to a CFG. This can fail as indicated by a NULL return.
  HCFGrammar *grammar = h_cfgrammar(mm__, parser);
  if(grammar == NULL)
    return -1;                  // -> Backend unsuitable for this parser.

  // TODO: eliminate common prefixes
  // TODO: eliminate left recursion
  // TODO: avoid conflicts by splitting occurances?

  // generate table and store in parser->data.
  HLLTable *table = h_lltable_new(mm__);
  if(fill_table(grammar, table) < 0) {
    // the table was ambiguous
    h_cfgrammar_free(grammar);
    h_lltable_free(table);
    return -1;
  }
  parser->data = table;

  // free grammar and its arena.
  // desugared parsers (HCFChoice and HCFSequence) are unaffected by this.
  h_cfgrammar_free(grammar);

  return 0;
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
