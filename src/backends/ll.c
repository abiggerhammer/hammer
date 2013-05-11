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
  HCFChoice  *start;    // start symbol
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

/* Generate the LL parse table from the given grammar.
 * Returns -1 on error, 0 on success.
 */
static int fill_table(HCFGrammar *g, HLLTable *table)
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

      // create table row for this nonterminal
      HHashTable *row = h_hashtable_new(table->arena, h_eq_ptr, h_hash_ptr);
      h_hashtable_put(table->rows, a, row);

      // iterate over a's productions
      HCFSequence **s;
      for(s = a->seq; *s; s++) {
        // record this production in row as appropriate
        // this can signal an ambiguity conflict.
        // NB we don't worry about deallocating anything, h_ll_compile will
        //    delete the whole arena for us.
        if(fill_table_row(g, row, a, *s) < 0)
          return -1;
      }
    }
  }
  
  return 0;
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

HParseResult *h_ll_parse(HAllocator* mm__, const HParser* parser, HParseState* state)
{
  const HLLTable *table = parser->data;
  HArena *arena = state->arena;
  HSlist *stack = h_slist_new(arena);
  HCountedArray *seq = h_carray_new(arena); // accumulates current parse result

  // in order to construct the parse tree, we delimit the symbol stack into
  // frames corresponding to production right-hand sides. since only left-most
  // derivations are produced this linearization is unique.
  // the 'mark' allocated below simply reserves a memory address to use as the
  // frame delimiter.
  // also on the stack below the mark, we store the previously accumulated
  // value for the surrounding production.
  void *mark = h_arena_malloc(arena, 1);

  // initialize with the start symbol on the stack.
  h_slist_push(stack, table->start);

  HCFToken lookahead = 0;   // 0 = empty

  // when we empty the stack, the parse is complete.
  while(!h_slist_empty(stack)) {
    // fill up lookahead buffer as required
    if(lookahead == 0) {
        uint8_t c = h_read_bits(&state->input_stream, 8, false);
        if(state->input_stream.overrun)
          lookahead = end_token;
        else
          lookahead = char_token(c);
    }

    // pop top of stack and check for frame delimiter
    HCFChoice *x = h_slist_pop(stack);
    assert(x != NULL);
    if(x == mark)
    {
      // hit stack frame boundary

      // wrap the accumulated parse result, this sequence is finished
      HParsedToken *tok = a_new(HParsedToken, 1);
      tok->token_type = TT_SEQUENCE;
      tok->seq = seq;
      // XXX tok->index and tok->bit_offset (don't take directly from stream, cuz peek!)

      // call validation and semantic action, if present
      if(x->pred && !x->pred(make_result(state, tok)))
        return NULL;    // validation failed -> no parse
      if(x->action)
        tok = (HParsedToken *)x->action(make_result(state, tok));

      // result becomes next left-most element of higher-level sequence
      seq = h_slist_pop(stack);
      h_carray_append(seq, tok);
    }
    else if(x->type == HCF_CHOICE)
    {
      // x is a nonterminal; apply the appropriate production

      // push stack frame
      h_slist_push(stack, seq);   // save current partial value
      h_slist_push(stack, mark);  // frame delimiter

      // open a fresh result sequence
      seq = h_carray_new(arena);

      // look up applicable production in parse table
      const HCFSequence *p = h_ll_lookup(table, x, lookahead);

      // push production's rhs onto the stack (in reverse order)
      HCFChoice **s;
      for(s = p->items; *s; s++);
      for(s--; s >= p->items; s--)
        h_slist_push(stack, *s);
    }
    else
    {
      // x is a terminal, or simple charset; match against input

      // consume the input token
      HCFToken input = lookahead;
      lookahead = 0;

      HParsedToken *tok;
      switch(x->type) {
      case HCF_END:
        if(input != end_token)
          return NULL;
        tok = NULL;
        break;

      case HCF_CHAR:
        if(input != char_token(x->chr))
          return NULL;
        tok = a_new(HParsedToken, 1);
        tok->token_type = TT_UINT;
        tok->uint = x->chr;
        break;

      case HCF_CHARSET:
        if(input == end_token)
          return NULL;
        if(!charset_isset(x->charset, token_char(input)))
          return NULL;
        tok = a_new(HParsedToken, 1);
        tok->token_type = TT_UINT;
        tok->uint = token_char(input);
        break;

      default: // should not be reached
        assert_message(0, "unknown HCFChoice type");
        return NULL;
      }

      // XXX tok->index and tok->bit_offset (don't take directly from stream, cuz peek!)

      // call validation and semantic action, if present
      if(x->pred && !x->pred(make_result(state, tok)))
        return NULL;    // validation failed -> no parse
      if(x->action)
        tok = (HParsedToken *)x->action(make_result(state, tok));

      // append to result sequence
      h_carray_append(seq, tok);
    }
  }

  // since we started with a single nonterminal on the stack, seq should
  // contain exactly the parse result.
  assert(seq->used == 1);
  return make_result(state, seq->elements[0]);
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
