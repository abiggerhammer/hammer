#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"

static const size_t DEFAULT_KMAX = 1;


/* Generating the LL(k) parse table */

/* Maps each nonterminal (HCFChoice) of the grammar to a HStringMap that
 * maps lookahead strings to productions (HCFSequence).
 */
typedef struct HLLkTable_ {
  size_t     kmax;
  HHashTable *rows;
  HCFChoice  *start;    // start symbol
  HArena     *arena;
  HAllocator *mm__;
} HLLkTable;


/* Interface to look up an entry in the parse table. */
const HCFSequence *h_llk_lookup(const HLLkTable *table, const HCFChoice *x,
                                const HInputStream *stream)
{
  const HStringMap *row = h_hashtable_get(table->rows, x);
  assert(row != NULL);  // the table should have one row for each nonterminal

  assert(!row->epsilon_branch); // would match without looking at the input
                                // XXX cases where this could be useful?

  return h_stringmap_get_lookahead(row, *stream);
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

void *const CONFLICT = (void *)(uintptr_t)(-1);

// helper for stringmap_merge
static void *combine_entries(HHashSet *workset, void *dst, const void *src)
{
  assert(dst != NULL);
  assert(src != NULL);

  if(dst == CONFLICT) {                 // previous conflict
    h_hashset_put(workset, src);
  } else if(dst != src) {               // new conflict
    h_hashset_put(workset, dst);
    h_hashset_put(workset, src);
    dst = CONFLICT;
  }

  return dst;
}

// add the mappings of src to dst, marking conflicts and adding the conflicting
// values to workset.
static void stringmap_merge(HHashSet *workset, HStringMap *dst, HStringMap *src)
{
  if(src->epsilon_branch) {
    if(dst->epsilon_branch)
      dst->epsilon_branch =
        combine_entries(workset, dst->epsilon_branch, src->epsilon_branch);
    else
      dst->epsilon_branch = src->epsilon_branch;
  } else {
    // if there is a non-conflicting value on the left (dst) side, it means
    // that prediction is already unambiguous. we can drop the right (src)
    // side we were going to extend with.
    if(dst->epsilon_branch && dst->epsilon_branch != CONFLICT)
      return;
  }

  if(src->end_branch) {
    if(dst->end_branch)
      dst->end_branch =
        combine_entries(workset, dst->end_branch, src->end_branch);
    else
      dst->end_branch = src->end_branch;
  }

  // iterate over src->char_branches
  const HHashTable *ht = src->char_branches;
  for(size_t i=0; i < ht->capacity; i++) {
    for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;

      HCharKey c = (HCharKey)hte->key;
      HStringMap *src_ = hte->value;

      if(src_) {
        HStringMap *dst_ = h_hashtable_get(dst->char_branches, (void *)c);
        if(dst_) {
          stringmap_merge(workset, dst_, src_);
        } else {
          if(src_->arena != dst->arena)
            src_ = h_stringmap_copy(dst->arena, src_);
          h_hashtable_put(dst->char_branches, (void *)c, src_);
        }
      }
    }
  }
}

/* Generate entries for the productions of A in the given table row. */
static int fill_table_row(size_t kmax, HCFGrammar *g, HStringMap *row,
                          const HCFChoice *A)
{
  HHashSet *workset;

  // initialize working set to the productions of A
  workset = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  for(HCFSequence **s = A->seq; *s; s++)
    h_hashset_put(workset, *s);

  // run until workset exhausted or kmax hit
  size_t k;
  for(k=1; k<=kmax; k++) {
    // allocate a fresh workset for the next round
    HHashSet *nextset = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);

    // iterate over the productions in workset...
    const HHashTable *ht = workset;
    for(size_t i=0; i < ht->capacity; i++) {
      for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
        if(hte->key == NULL)
          continue;

        HCFSequence *rhs = (void *)hte->key;
        assert(rhs != NULL);
        assert(rhs != CONFLICT);  // just to be sure there's no mixup

        // calculate predict set; let values map to rhs
        HStringMap *pred = h_predict(k, g, A, rhs);
        h_stringmap_replace(pred, NULL, rhs);

        // merge predict set into the row
        // accumulates conflicts in new workset
        stringmap_merge(nextset, row, pred);
      }
    }

    // switch to the updated workset
    h_hashset_free(workset);
    workset = nextset;

    // if the workset is empty, row is without conflict; we're done
    if(h_hashset_empty(workset))
      break;

    // clear conflict markers for next iteration
    h_stringmap_replace(row, CONFLICT, NULL);
  }

  h_hashset_free(workset);
  return (k>kmax)? -1 : 0;
}

/* Generate the LL(k) parse table from the given grammar.
 * Returns -1 on error, 0 on success.
 */
static int fill_table(size_t kmax, HCFGrammar *g, HLLkTable *table)
{
  table->kmax = kmax;
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
      HStringMap *row = h_stringmap_new(table->arena);
      h_hashtable_put(table->rows, a, row);

      if(fill_table_row(kmax, g, row, a) < 0) {
        // unresolvable conflicts in row
        // NB we don't worry about deallocating anything, h_llk_compile will
        //    delete the whole arena for us.
        return -1;
      }
    }
  }
  
  return 0;
}

int h_llk_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  size_t kmax = params? (uintptr_t)params : DEFAULT_KMAX;
  assert(kmax>0);

  // Convert parser to a CFG. This can fail as indicated by a NULL return.
  HCFGrammar *grammar = h_cfgrammar(mm__, parser);
  if(grammar == NULL)
    return -1;                  // -> Backend unsuitable for this parser.

  // TODO: eliminate common prefixes
  // TODO: eliminate left recursion
  // TODO: avoid conflicts by splitting occurances?

  // generate table and store in parser->backend_data.
  HLLkTable *table = h_llktable_new(mm__);
  if(fill_table(kmax, grammar, table) < 0) {
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

typedef struct {
  HArena *arena;        // will hold the results
  HArena *tarena;       // tmp, deleted after parse
  HSlist *stack;
  HCountedArray *seq;   // accumulates current parse result

  uint8_t *buf;         // for lookahead across chunk boundaries
                        // allocated to size 2*kmax
                        // new chunk starts at index kmax
                        // ( 0  ... kmax ...  2*kmax-1 )
                        //   \_old_/\______new_______/
  HInputStream win;     // win.length is set to 0 when not in use
} HLLkState;

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
static void const * const MARK = &MARK; // stack frame delimiter

static HLLkState *llk_parse_start_(HAllocator* mm__, const HParser* parser)
{
  const HLLkTable *table = parser->backend_data;
  assert(table != NULL);

  HLLkState *s = h_new(HLLkState, 1);
  s->arena  = h_new_arena(mm__, 0);
  s->tarena = h_new_arena(mm__, 0);
  s->stack  = h_slist_new(s->tarena);
  s->seq    = h_carray_new(s->arena);
  s->buf    = h_arena_malloc(s->tarena, 2 * table->kmax);

  s->win.input  = s->buf;
  s->win.length = 0;      // unused

  // initialize with the start symbol on the stack.
  h_slist_push(s->stack, table->start);

  return s;
}

// helper: add new input to the lookahead window
static void append_win(size_t kmax, HLLkState *s, HInputStream *stream)
{
  assert(stream->bit_offset == 0);
  assert(s->win.input == s->buf);
  assert(s->win.length == kmax);
  assert(s->win.index < kmax);

  size_t n = stream->length - stream->index;  // bytes to copy
  if(n > kmax)
    n = kmax;

  memcpy(s->buf + kmax, stream->input + stream->index, n);
  s->win.length += n;
}

// helper: save old input to the lookahead window
static void save_win(size_t kmax, HLLkState *s, HInputStream *stream)
{
  assert(stream->bit_offset == 0);

  size_t len = stream->length - stream->index;
  assert(len < kmax);

  if(len == 0) {
    // stream empty? nothing to do.
    return;
  } else if(s->win.length > 0) {
    // window active? should contain all of stream.
    assert(s->win.length == kmax + len);
    assert(s->win.index <= kmax);

    // shift contents down:
    //
    //   (0                 kmax            )
    //         ...   \_old_/\_new_/   ...
    //
    //   (0                 kmax            )
    //    ... \_old_/\_new_/       ...
    //
    s->win.pos += len;  // position of the window shifts up
    len = s->win.length - s->win.index;
    assert(len <= kmax);
    memmove(s->buf + kmax - len, s->buf + s->win.index, len);
  } else {
    // window not active? save stream to window.
    // buffer starts kmax bytes below chunk boundary
    s->win.pos = stream->pos - kmax;
    memcpy(s->buf + kmax - len, stream->input + stream->index, len);
  }

  // metadata
  s->win = *stream;
  s->win.input  = s->buf;
  s->win.index  = kmax - len;
  s->win.length = kmax;
}

// returns partial result or NULL (no parse)
static HCountedArray *llk_parse_chunk_(HLLkState *s, const HParser* parser,
                                       HInputStream* chunk)
{
  HParsedToken *tok = NULL;   // will hold result token
  HCFChoice *x = NULL;        // current symbol (from top of stack)
  HInputStream *stream;

  assert(chunk->index == 0);
  assert(chunk->bit_offset == 0);

  const HLLkTable *table = parser->backend_data;
  assert(table != NULL);

  HArena *arena = s->arena;
  HArena *tarena = s->tarena;
  HSlist *stack = s->stack;
  HCountedArray *seq = s->seq;
  size_t kmax = table->kmax;

  if(!seq)
    return NULL;  // parse already failed

  if(s->win.length > 0) {
    append_win(kmax, s, chunk);
    stream = &s->win;
  } else {
    stream = chunk;
  }

  // when we empty the stack, the parse is complete.
  while(!h_slist_empty(stack)) {
    tok = NULL;

    // pop top of stack for inspection
    x = h_slist_pop(stack);
    assert(x != NULL);

    if(x != MARK && x->type == HCF_CHOICE) {
      // x is a nonterminal; apply the appropriate production and continue

      // look up applicable production in parse table
      const HCFSequence *p = h_llk_lookup(table, x, stream);
      if(p == NULL)
        goto no_parse;
      if(p == NEED_INPUT) {
        save_win(kmax, s, chunk);
        goto need_input;
      }

      // an infinite loop case that shouldn't happen
      assert(!p->items[0] || p->items[0] != x);

      // push stack frame
      h_slist_push(stack, seq);           // save current partial value
      h_slist_push(stack, x);             // save the nonterminal
      h_slist_push(stack, (void *)MARK);  // frame delimiter

      // open a fresh result sequence
      seq = h_carray_new(arena);

      // push production's rhs onto the stack (in reverse order)
      HCFChoice **s;
      for(s = p->items; *s; s++);
      for(s--; s >= p->items; s--)
        h_slist_push(stack, *s);

      continue; // no result to record
    }

    // the top of stack is such that there will be a result...
    tok = h_arena_malloc(arena, sizeof(HParsedToken));
    if(x == MARK) {
      // hit stack frame boundary...
      // wrap the accumulated parse result, this sequence is finished
      tok->token_type = TT_SEQUENCE;
      tok->seq = seq;
      // XXX would have to set token pos but we've forgotten pos of seq

      // recover original nonterminal and result sequence
      x   = h_slist_pop(stack);
      seq = h_slist_pop(stack);
      // tok becomes next left-most element of higher-level sequence
    }
    else {
      // x is a terminal or simple charset; match against input

      tok->index = stream->pos + stream->index;
      tok->bit_offset = stream->bit_offset;

      // consume the input token
      uint8_t input = h_read_bits(stream, 8, false);

      // when old chunk consumed from window, switch to new chunk
      if(s->win.length > 0 && s->win.index >= kmax) {
        s->win.length = 0;  // disable the window
        stream = chunk;
      }

      switch(x->type) {
      case HCF_END:
        if(!stream->overrun)
          goto no_parse;
        if(!stream->last_chunk)
          goto need_input;
        h_arena_free(arena, tok);
        tok = NULL;
        break;

      case HCF_CHAR:
        if(stream->overrun)
          goto need_input;
        if(input != x->chr)
          goto no_parse;
        tok->token_type = TT_UINT;
        tok->uint = x->chr;
        break;

      case HCF_CHARSET:
        if(stream->overrun)
          goto need_input;
        if(!charset_isset(x->charset, input))
          goto no_parse;
        tok->token_type = TT_UINT;
        tok->uint = input;
        break;

      default: // should not be reached
        assert_message(0, "unknown HCFChoice type");
        goto no_parse;
      }
    }

    // 'tok' has been parsed; process it

    // perform token reshape if indicated
    if(x->reshape) {
      HParsedToken *t = x->reshape(make_result(arena, tok), x->user_data);
      if(t) {
        t->index = tok->index;
        t->bit_offset = tok->bit_offset;
      } else {
        h_arena_free(arena, tok);
      }
      tok = t;
    }

    // call validation and semantic action, if present
    if(x->pred && !x->pred(make_result(tarena, tok), x->user_data))
      goto no_parse;    // validation failed -> no parse
    if(x->action)
      tok = (HParsedToken *)x->action(make_result(arena, tok), x->user_data);

    // append to result sequence
    h_carray_append(seq, tok);
  }

  // success
  // since we started with a single nonterminal on the stack, seq should
  // contain exactly the parse result.
  assert(seq->used == 1);
  return seq;

 no_parse:
  h_delete_arena(arena);
  s->arena = NULL;
  return NULL;

 need_input:
  if(stream->last_chunk)
    goto no_parse;
  if(tok)
    h_arena_free(arena, tok);   // no result, yet
  h_slist_push(stack, x);       // try this symbol again next time
  return seq;
}

static HParseResult *llk_parse_finish_(HAllocator *mm__, HLLkState *s)
{
  HParseResult *res = NULL;

  if(s->seq) {
    assert(s->seq->used == 1);
    res = make_result(s->arena, s->seq->elements[0]);
  }

  h_delete_arena(s->tarena);
  h_free(s);
  return res;
}

HParseResult *h_llk_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  HLLkState *s = llk_parse_start_(mm__, parser);

  assert(stream->last_chunk);
  s->seq = llk_parse_chunk_(s, parser, stream);

  HParseResult *res = llk_parse_finish_(mm__, s);
  if(res)
    res->bit_length = stream->index * 8 + stream->bit_offset;

  return res;
}

void h_llk_parse_start(HSuspendedParser *s)
{
  s->backend_state = llk_parse_start_(s->mm__, s->parser);
}

bool h_llk_parse_chunk(HSuspendedParser *s, HInputStream *input)
{
  HLLkState *state = s->backend_state;

  state->seq = llk_parse_chunk_(state, s->parser, input);

  return (state->seq == NULL || h_slist_empty(state->stack));
}

HParseResult *h_llk_parse_finish(HSuspendedParser *s)
{
  return llk_parse_finish_(s->mm__, s->backend_state);
}


HParserBackendVTable h__llk_backend_vtable = {
  .compile = h_llk_compile,
  .parse = h_llk_parse,
  .free = h_llk_free,

  .parse_start = h_llk_parse_start,
  .parse_chunk = h_llk_parse_chunk,
  .parse_finish = h_llk_parse_finish
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
  printf("derive epsilon: ");
  h_pprint_symbolset(stdout, g, g->geneps, 0);
  printf("first(A) = ");
  h_pprint_stringset(stdout, h_first(3, g, g->start), 0);
  //  printf("follow(C) = ");
  //  h_pprint_stringset(stdout, h_follow(3, g, h_desugar(&system_allocator, NULL, c)), 0);

  if(h_compile(p, PB_LLk, (void *)3)) {
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
