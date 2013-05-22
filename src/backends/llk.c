#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"

// XXX despite the names, this is all LL(1) right now. TODO


/* Generating the LL(k) parse table */

/* Maps each nonterminal (HCFChoice) of the grammar to another hash table that
 * maps lookahead tokens (HCFToken) to productions (HCFSequence).
 */
typedef struct HLLkTable_ {
  HHashTable *rows;
  HCFChoice  *start;    // start symbol
  HArena     *arena;
  HAllocator *mm__;
} HLLkTable;


// XXX adaptation to LL(1), to be removed
typedef HCharKey HCFToken;
static const HCFToken end_token = 0x200;
#define char_token char_key

/* Interface to look up an entry in the parse table. */
const HCFSequence *h_llk_lookup(const HLLkTable *table, const HCFChoice *x,
                                HInputStream lookahead)
{
  // note the lookahead stream is passed by value, i.e. a copy.
  // reading bits from it does not consume them from the real input.
  HCFToken tok;
  uint8_t c = h_read_bits(&lookahead, 8, false);
  if(lookahead.overrun)
    tok = end_token;
  else
    tok = char_token(c);

  const HHashTable *row = h_hashtable_get(table->rows, x);
  assert(row != NULL);  // the table should have one row for each nonterminal

  const HCFSequence *production = h_hashtable_get(row, (void *)tok);
  return production;
}

/* Allocate a new parse table. */
HLLkTable *h_llktable_new(HAllocator *mm__)
{
  // NB the parse table gets an arena separate from the grammar so we can free
  //    the latter after table generation.
  HArena *arena = h_new_arena(mm__, 0);    // default blocksize
  assert(arena != NULL);
  HHashTable *rows = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);
  assert(rows != NULL);

  HLLkTable *table = h_new(HLLkTable, 1);
  assert(table != NULL);
  table->mm__  = mm__;
  table->arena = arena;
  table->rows  = rows;

  return table;
}

void h_llktable_free(HLLkTable *table)
{
  if(table == NULL)
    return;
  HAllocator *mm__ = table->mm__;
  h_delete_arena(table->arena);
  h_free(table);
}

/* Compute the predict set of production "A -> rhs". */
HHashSet *h_predict(HCFGrammar *g, const HCFChoice *A, const HCFSequence *rhs)
{
  // predict(A -> rhs) = first(rhs) u follow(A)  if "" can be derived from rhs
  // predict(A -> rhs) = first(rhs)              otherwise
  const HCFStringMap *first_rhs = h_first_seq(1, g, rhs->items);
  const HCFStringMap *follow_A = h_follow(1, g, A);
  HHashSet *ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);

  h_hashset_put_all(ret, first_rhs->char_branches);
  if(first_rhs->end_branch)
    h_hashset_put(ret, (void *)end_token);

  if(h_derives_epsilon_seq(g, rhs->items)) {
    h_hashset_put_all(ret, follow_A->char_branches);
    if(follow_A->end_branch)
      h_hashset_put(ret, (void *)end_token);
  }

  return ret;
}

/* Generate entries for the production "A -> rhs" in the given table row. */
static
int fill_table_row(HCFGrammar *g, HHashTable *row,
                   const HCFChoice *A, HCFSequence *rhs)
{
  // iterate over predict(A -> rhs)
  HHashSet *pred = h_predict(g, A, rhs);

  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < pred->capacity; i++) {
    for(hte = &pred->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      HCFToken x = (uintptr_t)hte->key;

      if(h_hashtable_present(row, (void *)x))
        return -1;  // table would be ambiguous

      h_hashtable_put(row, (void *)x, rhs);
    }
  }

  return 0;
}

/* Generate the LL(k) parse table from the given grammar.
 * Returns -1 on error, 0 on success.
 */
static int fill_table(HCFGrammar *g, HLLkTable *table)
{
  table->start = g->start;

  // iterate over g->nts
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      const HCFChoice *a = hte->key;        // production's left-hand symbol
      assert(a->type == HCF_CHOICE);

      // create table row for this nonterminal
      HHashTable *row = h_hashtable_new(table->arena, h_eq_ptr, h_hash_ptr);
      h_hashtable_put(table->rows, a, row);

      // iterate over a's productions
      HCFSequence **s;
      for(s = a->seq; *s; s++) {
        // record this production in row as appropriate
        // this can signal an ambiguity conflict.
        // NB we don't worry about deallocating anything, h_llk_compile will
        //    delete the whole arena for us.
        if(fill_table_row(g, row, a, *s) < 0)
          return -1;
      }
    }
  }
  
  return 0;
}

int h_llk_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // Convert parser to a CFG. This can fail as indicated by a NULL return.
  HCFGrammar *grammar = h_cfgrammar(mm__, parser);
  if(grammar == NULL)
    return -1;                  // -> Backend unsuitable for this parser.

  // TODO: eliminate common prefixes
  // TODO: eliminate left recursion
  // TODO: avoid conflicts by splitting occurances?

  // generate table and store in parser->backend_data.
  HLLkTable *table = h_llktable_new(mm__);
  if(fill_table(grammar, table) < 0) {
    // the table was ambiguous
    h_cfgrammar_free(grammar);
    h_llktable_free(table);
    return -1;
  }
  parser->backend_data = table;

  // free grammar and its arena.
  // desugared parsers (HCFChoice and HCFSequence) are unaffected by this.
  h_cfgrammar_free(grammar);

  return 0;
}

void h_llk_free(HParser *parser)
{
  HLLkTable *table = parser->backend_data;
  h_llktable_free(table);
  parser->backend_data = NULL;
  parser->backend = PB_PACKRAT;
}



/* LL(k) driver */

HParseResult *h_llk_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  const HLLkTable *table = parser->backend_data;
  assert(table != NULL);

  HArena *arena  = h_new_arena(mm__, 0);    // will hold the results
  HArena *tarena = h_new_arena(mm__, 0);    // tmp, deleted after parse
  HSlist *stack  = h_slist_new(tarena);
  HCountedArray *seq = h_carray_new(arena); // accumulates current parse result

  // in order to construct the parse tree, we delimit the symbol stack into
  // frames corresponding to production right-hand sides. since only left-most
  // derivations are produced this linearization is unique.
  // the 'mark' allocated below simply reserves a memory address to use as the
  // frame delimiter.
  // nonterminals, instead of being popped and forgotten, are put back onto the
  // stack below the mark to tell us which validations and semantic actions to
  // execute on their corresponding result.
  // also on the stack below the mark, we store the previously accumulated
  // value for the surrounding production.
  void *mark = h_arena_malloc(tarena, 1);

  // initialize with the start symbol on the stack.
  h_slist_push(stack, table->start);

  // when we empty the stack, the parse is complete.
  while(!h_slist_empty(stack)) {
    // pop top of stack for inspection
    HCFChoice *x = h_slist_pop(stack);
    assert(x != NULL);

    if(x != mark && x->type == HCF_CHOICE) {
      // x is a nonterminal; apply the appropriate production and continue

      // push stack frame
      h_slist_push(stack, seq);   // save current partial value
      h_slist_push(stack, x);     // save the nonterminal
      h_slist_push(stack, mark);  // frame delimiter

      // open a fresh result sequence
      seq = h_carray_new(arena);

      // look up applicable production in parse table
      const HCFSequence *p = h_llk_lookup(table, x, *stream);
      if(p == NULL)
        goto no_parse;

      // push production's rhs onto the stack (in reverse order)
      HCFChoice **s;
      for(s = p->items; *s; s++);
      for(s--; s >= p->items; s--)
        h_slist_push(stack, *s);

      continue; // no result to record
    }

    // the top of stack is such that there will be a result...
    HParsedToken *tok;  // will hold result token
    if(x == mark) {
      // hit stack frame boundary...
      // wrap the accumulated parse result, this sequence is finished
      tok = h_arena_malloc(arena, sizeof(HParsedToken));
      tok->token_type = TT_SEQUENCE;
      tok->seq = seq;

      // recover original nonterminal and result sequence
      x   = h_slist_pop(stack);
      seq = h_slist_pop(stack);
      // tok becomes next left-most element of higher-level sequence
    }
    else {
      // x is a terminal or simple charset; match against input

      // consume the input token
      uint8_t input = h_read_bits(stream, 8, false);

      switch(x->type) {
      case HCF_END:
        if(!stream->overrun)
          goto no_parse;
        tok = NULL;
        break;

      case HCF_CHAR:
        if(input != x->chr)
          goto no_parse;
        tok = h_arena_malloc(arena, sizeof(HParsedToken));
        tok->token_type = TT_UINT;
        tok->uint = x->chr;
        break;

      case HCF_CHARSET:
        if(stream->overrun)
          goto no_parse;
        if(!charset_isset(x->charset, input))
          goto no_parse;
        tok = h_arena_malloc(arena, sizeof(HParsedToken));
        tok->token_type = TT_UINT;
        tok->uint = input;
        break;

      default: // should not be reached
        assert_message(0, "unknown HCFChoice type");
        goto no_parse;
      }
    }

    // 'tok' has been parsed; process it

    // XXX set tok->index and tok->bit_offset (don't take directly from stream, cuz peek!)

    // perform token reshape if indicated
    if(x->reshape)
      tok = (HParsedToken *)x->reshape(make_result(arena, tok));

    // call validation and semantic action, if present
    if(x->pred && !x->pred(make_result(tarena, tok)))
      goto no_parse;    // validation failed -> no parse
    if(x->action)
      tok = (HParsedToken *)x->action(make_result(arena, tok));

    // append to result sequence
    h_carray_append(seq, tok);
  }

  // since we started with a single nonterminal on the stack, seq should
  // contain exactly the parse result.
  assert(seq->used == 1);
  h_delete_arena(tarena);
  return make_result(arena, seq->elements[0]);

  no_parse:
    h_delete_arena(tarena);
    h_delete_arena(arena);
    return NULL;
}



HParserBackendVTable h__llk_backend_vtable = {
  .compile = h_llk_compile,
  .parse = h_llk_parse,
  .free = h_llk_free
};




// dummy!
int test_llk(void)
{
  /* for k=2:

     S -> A | B
     A -> X Y a
     B -> Y b
     X -> x | ''
     Y -> y         -- for k=3 use "yy"
  */

  HParser *c = h_many(h_ch('x'));
  HParser *q = h_sequence(c, h_ch('y'), NULL);
  HParser *p = h_choice(q, h_end_p(), NULL);

  HCFGrammar *g = h_cfgrammar(&system_allocator, p);

  if(g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }

  h_pprint_grammar(stdout, g, 0);
  printf("generate epsilon: ");
  h_pprint_symbolset(stdout, g, g->geneps, 0);
  printf("first(A) = ");
  h_pprint_stringset(stdout, g, h_first(1, g, g->start), 0);
  printf("follow(C) = ");
  h_pprint_stringset(stdout, g, h_follow(1, g, h_desugar(&system_allocator, c)), 0);

  h_compile(p, PB_LLk, NULL);

  HParseResult *res = h_parse(p, (uint8_t *)"xxy", 3);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
