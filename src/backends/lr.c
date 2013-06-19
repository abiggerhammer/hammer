#include <assert.h>
#include "../parsers/parser_internal.h"
#include "lr.h"



/* Comparison and hashing functions */

// compare symbols - terminals by value, others by pointer
bool h_eq_symbol(const void *p, const void *q)
{
  const HCFChoice *x=p, *y=q;
  return (x==y
          || (x->type==HCF_END && y->type==HCF_END)
          || (x->type==HCF_CHAR && y->type==HCF_CHAR && x->chr==y->chr));
}

// hash symbols - terminals by value, others by pointer
HHashValue h_hash_symbol(const void *p)
{
  const HCFChoice *x=p;
  if(x->type == HCF_END)
    return 0;
  else if(x->type == HCF_CHAR)
    return x->chr * 33;
  else
    return h_hash_ptr(p);
}

// compare LR items by value
static bool eq_lr_item(const void *p, const void *q)
{
  const HLRItem *a=p, *b=q;

  if(!h_eq_symbol(a->lhs, b->lhs)) return false;
  if(a->mark != b->mark) return false;
  if(a->len != b->len) return false;

  for(size_t i=0; i<a->len; i++)
    if(!h_eq_symbol(a->rhs[i], b->rhs[i])) return false;

  return true;
}

// hash LALR items
static inline HHashValue hash_lr_item(const void *p)
{
  const HLRItem *x = p;
  HHashValue hash = 0;

  hash += h_hash_symbol(x->lhs);
  for(HCFChoice **p=x->rhs; *p; p++)
    hash += h_hash_symbol(*p);
  hash += x->mark;

  return hash;
}

// compare item sets (DFA states)
bool h_eq_lr_itemset(const void *p, const void *q)
{
  return h_hashset_equal(p, q);
}

// hash LR item sets (DFA states) - hash the elements and sum
HHashValue h_hash_lr_itemset(const void *p)
{
  HHashValue hash = 0;

  H_FOREACH_KEY((const HHashSet *)p, HLRItem *item)
    hash += hash_lr_item(item);
  H_END_FOREACH

  return hash;
}

bool h_eq_transition(const void *p, const void *q)
{
  const HLRTransition *a=p, *b=q;
  return (a->from == b->from && a->to == b->to && h_eq_symbol(a->symbol, b->symbol));
}

HHashValue h_hash_transition(const void *p)
{
  const HLRTransition *t = p;
  return (h_hash_symbol(t->symbol) + t->from + t->to); // XXX ?
}



/* Constructors */

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

  return ret;
}

HLRState *h_lrstate_new(HArena *arena)
{
  return h_hashset_new(arena, eq_lr_item, hash_lr_item);
}

HLRTable *h_lrtable_new(HAllocator *mm__, size_t nrows)
{
  HArena *arena = h_new_arena(mm__, 0);    // default blocksize
  assert(arena != NULL);

  HLRTable *ret = h_new(HLRTable, 1);
  ret->nrows = nrows;
  ret->rows = h_arena_malloc(arena, nrows * sizeof(HHashTable *));
  ret->forall = h_arena_malloc(arena, nrows * sizeof(HLRAction *));
  ret->inadeq = h_slist_new(arena);
  ret->arena = arena;
  ret->mm__ = mm__;

  for(size_t i=0; i<nrows; i++) {
    ret->rows[i] = h_hashtable_new(arena, h_eq_symbol, h_hash_symbol);
    ret->forall[i] = NULL;
  }

  return ret;
}

void h_lrtable_free(HLRTable *table)
{
  HAllocator *mm__ = table->mm__;
  h_delete_arena(table->arena);
  h_free(table);
}

HLRAction *h_shift_action(HArena *arena, size_t nextstate)
{
  HLRAction *action = h_arena_malloc(arena, sizeof(HLRAction));
  action->type = HLR_SHIFT;
  action->nextstate = nextstate;
  return action;
}

HLRAction *h_reduce_action(HArena *arena, const HLRItem *item)
{
  HLRAction *action = h_arena_malloc(arena, sizeof(HLRAction));
  action->type = HLR_REDUCE;
  action->production.lhs = item->lhs;
  action->production.length = item->len;
#ifndef NDEBUG
  action->production.rhs = item->rhs;
#endif
  return action;
}

// adds 'new' to the branches of 'action'
// returns a 'action' if it is already of type HLR_CONFLICT
// allocates a new HLRAction otherwise
HLRAction *h_lr_conflict(HArena *arena, HLRAction *action, HLRAction *new)
{
  if(action->type != HLR_CONFLICT) {
    HLRAction *old = action;
    action = h_arena_malloc(arena, sizeof(HLRAction));
    action->type = HLR_CONFLICT;
    action->branches = h_slist_new(arena);
    h_slist_push(action->branches, old);
  }

  assert(action->type == HLR_CONFLICT);
  h_slist_push(action->branches, new);

  return action;
}



/* LR driver */

const HLRAction *
h_lr_lookup(const HLRTable *table, size_t state, const HCFChoice *symbol)
{
  assert(state < table->nrows);
  if(table->forall[state]) {
    assert(h_hashtable_empty(table->rows[state]));  // that would be a conflict
    return table->forall[state];
  } else {
    return h_hashtable_get(table->rows[state], symbol);
  }
}

HLREngine *h_lrengine_new(HArena *arena, HArena *tarena, const HLRTable *table)
{
  HLREngine *engine = h_arena_malloc(tarena, sizeof(HLREngine));

  engine->table = table;
  engine->left = h_slist_new(tarena);
  engine->right = h_slist_new(tarena);
  engine->state = 0;
  engine->arena = arena;
  engine->tarena = tarena;

  return engine;
}

const HLRAction *h_lrengine_action(HLREngine *engine, HInputStream *stream)
{
  HSlist *right = engine->right;
  HArena *arena = engine->arena;
  HArena *tarena = engine->tarena;

  // make sure there is input on the right stack
  if(h_slist_empty(right)) {
    // XXX use statically-allocated terminal symbols
    HCFChoice *x = h_arena_malloc(tarena, sizeof(HCFChoice));
    HParsedToken *v;

    uint8_t c = h_read_bits(stream, 8, false);

    if(stream->overrun) {     // end of input
      x->type = HCF_END;
      v = NULL;
    } else {
      x->type = HCF_CHAR;
      x->chr = c;
      v = h_arena_malloc(arena, sizeof(HParsedToken));
      v->token_type = TT_UINT;
      v->uint = c;
    }

    h_slist_push(right, v);
    h_slist_push(right, x);
  }

  // peek at input symbol on the right side
  HCFChoice *symbol = right->head->elem;

  // table lookup
  const HLRAction *action = h_lr_lookup(engine->table, engine->state, symbol);

  return action;
}

// run LR parser for one round; returns false when finished
bool h_lrengine_step(HLREngine *engine, const HLRAction *action)
{
  // short-hand names
  HSlist *left = engine->left;
  HSlist *right = engine->right;
  HArena *arena = engine->arena;
  HArena *tarena = engine->tarena;

  if(action == NULL)
    return false;   // no handle recognizable in input, terminate

  assert(action->type == HLR_SHIFT || action->type == HLR_REDUCE);

  if(action->type == HLR_SHIFT) {
    h_slist_push(left, (void *)(uintptr_t)engine->state);
    h_slist_pop(right);                       // symbol (discard)
    h_slist_push(left, h_slist_pop(right));   // semantic value
    engine->state = action->nextstate;
  } else {
    assert(action->type == HLR_REDUCE);
    size_t len = action->production.length;
    HCFChoice *symbol = action->production.lhs;

    // semantic value of the reduction result
    HParsedToken *value = h_arena_malloc(arena, sizeof(HParsedToken));
    value->token_type = TT_SEQUENCE;
    value->seq = h_carray_new_sized(arena, len);
    
    // pull values off the left stack, rewinding state accordingly
    HParsedToken *v = NULL;
    for(size_t i=0; i<len; i++) {
      v = h_slist_pop(left);
      engine->state = (uintptr_t)h_slist_pop(left);

      // collect values in result sequence
      value->seq->elements[len-1-i] = v;
      value->seq->used++;
    }
    if(v) {
      // result position equals position of left-most symbol
      value->index = v->index;
      value->bit_offset = v->bit_offset;
    } else {
      // XXX how to get the position in this case?
    }

    // perform token reshape if indicated
    if(symbol->reshape)
      value = (HParsedToken *)symbol->reshape(make_result(arena, value));

    // call validation and semantic action, if present
    if(symbol->pred && !symbol->pred(make_result(tarena, value)))
      return false;     // validation failed -> no parse; terminate
    if(symbol->action)
      value = (HParsedToken *)symbol->action(make_result(arena, value));

    // push result (value, symbol) onto the right stack
    h_slist_push(right, value);
    h_slist_push(right, symbol);
  }

  return true;
}

HParseResult *h_lrengine_result(HLREngine *engine)
{
  // parsing was successful iff the start symbol is on top of the right stack
  if(h_slist_pop(engine->right) == engine->table->start) {
    // next on the right stack is the start symbol's semantic value
    assert(!h_slist_empty(engine->right));
    HParsedToken *tok = h_slist_pop(engine->right);
    return make_result(engine->arena, tok);
  } else {
    return NULL;
  }
}

HParseResult *h_lr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  HLRTable *table = parser->backend_data;
  if(!table)
    return NULL;

  HArena *arena  = h_new_arena(mm__, 0);    // will hold the results
  HArena *tarena = h_new_arena(mm__, 0);    // tmp, deleted after parse
  HLREngine *engine = h_lrengine_new(arena, tarena, table);

  // iterate engine to completion
  while(h_lrengine_step(engine, h_lrengine_action(engine, stream)));

  HParseResult *result = h_lrengine_result(engine);
  if(!result)
    h_delete_arena(arena);
  h_delete_arena(tarena);
  return result;
}



/* Pretty-printers */

void h_pprint_lritem(FILE *f, const HCFGrammar *g, const HLRItem *item)
{
  h_pprint_symbol(f, g, item->lhs);
  fputs(" ->", f);

  HCFChoice **x = item->rhs;
  HCFChoice **mark = item->rhs + item->mark;
  if(*x == NULL) {
    fputc('.', f);
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
  H_FOREACH_KEY(state, HLRItem *item)
    if(!first)
      for(unsigned int i=0; i<indent; i++) fputc(' ', f);
    first = false;
    h_pprint_lritem(f, g, item);
    fputc('\n', f);
  H_END_FOREACH
}

static void pprint_transition(FILE *f, const HCFGrammar *g, const HLRTransition *t)
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

void pprint_lraction(FILE *f, const HCFGrammar *g, const HLRAction *action)
{
  if(action->type == HLR_SHIFT) {
    fprintf(f, "s%lu", action->nextstate);
  } else {
    fputs("r(", f);
    h_pprint_symbol(f, g, action->production.lhs);
    fputs(" -> ", f);
#ifdef NDEBUG
    // if we can't print the production, at least print its length
    fprintf(f, "[%lu]", action->production.length);
#else
    HCFSequence seq = {action->production.rhs};
    h_pprint_sequence(f, g, &seq);
#endif
    fputc(')', f);
  }
}

void h_pprint_lrtable(FILE *f, const HCFGrammar *g, const HLRTable *table,
                      unsigned int indent)
{
  for(size_t i=0; i<table->nrows; i++) {
    for(unsigned int j=0; j<indent; j++) fputc(' ', f);
    fprintf(f, "%4lu:", i);
    if(table->forall[i]) {
      fputs(" - ", f);
      pprint_lraction(f, g, table->forall[i]);
      fputs(" -", f);
      if(!h_hashtable_empty(table->rows[i]))
        fputs(" !!", f);
    }
    H_FOREACH(table->rows[i], HCFChoice *symbol, HLRAction *action)
      fputc(' ', f);    // separator
      h_pprint_symbol(f, g, symbol);
      fputc(':', f);
      if(table->forall[i]) {
        fputc(action->type == HLR_SHIFT? 's' : 'r', f);
        fputc('/', f);
        fputc(table->forall[i]->type == HLR_SHIFT? 's' : 'r', f);
      } else {
        pprint_lraction(f, g, action);
      }
    H_END_FOREACH
    fputc('\n', f);
  }

#if 0
  fputs("inadeq=", f);
  for(HSlistNode *x=table->inadeq->head; x; x=x->next) {
    fprintf(f, "%lu ", (uintptr_t)x->elem);
  }
  fputc('\n', f);
#endif
}
