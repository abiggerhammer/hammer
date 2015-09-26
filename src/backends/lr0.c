#include <assert.h>
#include "lr.h"



/* Constructing the characteristic automaton (handle recognizer) */

static HLRItem *advance_mark(HArena *arena, const HLRItem *item)
{
  assert(item->rhs[item->mark] != NULL);
  HLRItem *ret = h_arena_malloc(arena, sizeof(HLRItem));
  *ret = *item;
  ret->mark++;
  return ret;
}

static void expand_to_closure(HCFGrammar *g, HHashSet *items)
{
  HAllocator *mm__ = g->mm__;
  HArena *arena = g->arena;
  HSlist *work = h_slist_new(arena);

  // initialize work list with items
  H_FOREACH_KEY(items, HLRItem *item)
    h_slist_push(work, (void *)item);
  H_END_FOREACH

  while(!h_slist_empty(work)) {
    const HLRItem *item = h_slist_pop(work);
    HCFChoice *sym = item->rhs[item->mark]; // symbol after mark

    // if there is a non-terminal after the mark, follow it
    // and add items corresponding to the productions of sym
    // NB: unlike LLk, we do consider HCF_CHARSET a non-terminal here
    if(sym != NULL) {
      if(sym->type == HCF_CHOICE) {
        for(HCFSequence **p=sym->seq; *p; p++) {
          HLRItem *it = h_lritem_new(arena, sym, (*p)->items, 0);
          if(!h_hashset_present(items, it)) {
            h_hashset_put(items, it);
            h_slist_push(work, it);
          }
        }
      } else if(sym->type == HCF_CHARSET) {
        for(unsigned int i=0; i<256; i++) {
          if(charset_isset(sym->charset, i)) {
            // XXX allocate these single-character symbols statically somewhere
            HCFChoice **rhs = h_new(HCFChoice *, 2);
            rhs[0] = h_new(HCFChoice, 1);
            rhs[0]->type = HCF_CHAR;
            rhs[0]->chr = i;
            rhs[1] = NULL;
            HLRItem *it = h_lritem_new(arena, sym, rhs, 0);
            h_hashset_put(items, it);
            // single-character item needs no further work
          }
        }
        // if sym is a non-terminal, we need a reshape on it
        // this seems as good a place as any to set it
        sym->reshape = h_act_first;
      }
    }
  }
}

HLRDFA *h_lr0_dfa(HCFGrammar *g)
{
  HArena *arena = g->arena;

  HHashSet *states = h_hashset_new(arena, h_eq_lr_itemset, h_hash_lr_itemset);
      // maps itemsets to assigned array indices
  HSlist *transitions = h_slist_new(arena);

  // list of states that need to be processed
  // to save lookups, we push two elements per state, the itemset and its
  // assigned index.
  HSlist *work = h_slist_new(arena);

  // make initial state (kernel)
  HLRState *start = h_lrstate_new(arena);
  assert(g->start->type == HCF_CHOICE);
  for(HCFSequence **p=g->start->seq; *p; p++)
    h_hashset_put(start, h_lritem_new(arena, g->start, (*p)->items, 0));
  expand_to_closure(g, start);
  h_hashtable_put(states, start, 0);
  h_slist_push(work, start);
  h_slist_push(work, 0);
  
  // while work to do (on some state)
  //   determine edge symbols
  //   for each edge symbol:
  //     advance respective items -> destination state (kernel)
  //     compute closure
  //     if destination is a new state:
  //       add it to state set
  //       add it to the work list
  //     add transition to it

  while(!h_slist_empty(work)) {
    size_t state_idx = (uintptr_t)h_slist_pop(work);
    HLRState *state = h_slist_pop(work);

    // maps edge symbols to neighbor states (item sets) of s
    HHashTable *neighbors = h_hashtable_new(arena, h_eq_symbol, h_hash_symbol);

    // iterate over state (closure) and generate neighboring sets
    H_FOREACH_KEY(state, HLRItem *item)
      HCFChoice *sym = item->rhs[item->mark]; // symbol after mark

      if(sym != NULL) { // mark was not at the end
        // find or create prospective neighbor set
        HLRState *neighbor = h_hashtable_get(neighbors, sym);
        if(neighbor == NULL) {
          neighbor = h_lrstate_new(arena);
          h_hashtable_put(neighbors, sym, neighbor);
        }

        // ...and add the advanced item to it
        h_hashset_put(neighbor, advance_mark(arena, item));
      }
    H_END_FOREACH

    // merge expanded neighbor sets into the set of existing states
    H_FOREACH(neighbors, HCFChoice *symbol, HLRState *neighbor)
      expand_to_closure(g, neighbor);

      // look up existing state, allocate new if not found
      size_t neighbor_idx;
      if(!h_hashset_present(states, neighbor)) {
        neighbor_idx = states->used;
        h_hashtable_put(states, neighbor, (void *)(uintptr_t)neighbor_idx);
        h_slist_push(work, neighbor);
        h_slist_push(work, (void *)(uintptr_t)neighbor_idx);
      } else {
        neighbor_idx = (uintptr_t)h_hashtable_get(states, neighbor);
      }

      // add transition "state --symbol--> neighbor"
      HLRTransition *t = h_arena_malloc(arena, sizeof(HLRTransition));
      t->from = state_idx;
      t->to = neighbor_idx;
      t->symbol = symbol;
      h_slist_push(transitions, t);
    H_END_FOREACH
  } // end while(work)

  // fill DFA struct
  HLRDFA *dfa = h_arena_malloc(arena, sizeof(HLRDFA));
  dfa->nstates = states->used;
  dfa->states = h_arena_malloc(arena, dfa->nstates*sizeof(HLRState *));
  H_FOREACH(states, HLRState *state, void *v)
    size_t idx = (uintptr_t)v;
    dfa->states[idx] = state;
  H_END_FOREACH
  dfa->transitions = transitions;

  return dfa;
}



/* LR(0) table generation */

static inline
void put_shift(HLRTable *table, size_t state, const HCFChoice *symbol,
               size_t nextstate)
{
  HLRAction *action = h_shift_action(table->arena, nextstate);

  switch(symbol->type) {
  case HCF_END:
    h_stringmap_put_end(table->tmap[state], action);
    break;
  case HCF_CHAR:
    h_stringmap_put_char(table->tmap[state], symbol->chr, action);
    break;
  default:
    // nonterminal case
    h_hashtable_put(table->ntmap[state], symbol, action);
  }
}

HLRTable *h_lr0_table(HCFGrammar *g, const HLRDFA *dfa)
{
  HAllocator *mm__ = g->mm__;

  HLRTable *table = h_lrtable_new(mm__, dfa->nstates);
  HArena *arena = table->arena;

  // remember start symbol
  table->start = g->start;

  // shift to the accepting end state for the start symbol
  put_shift(table, 0, g->start, HLR_SUCCESS);

  // add shift entries
  for(HSlistNode *x = dfa->transitions->head; x; x = x->next) {
    // for each transition x-A->y, add "shift, goto y" to table entry (x,A)
    HLRTransition *t = x->elem;

    put_shift(table, t->from, t->symbol, t->to);
  }

  // add reduce entries, record inadequate states
  for(size_t i=0; i<dfa->nstates; i++) {
    bool inadeq = false;

    // find reducible items in state
    H_FOREACH_KEY(dfa->states[i], HLRItem *item)
      if(item->mark == item->len) { // mark at the end
        HLRAction *reduce = h_reduce_action(arena, item);
        
        // check for reduce/reduce conflict on forall
        if(table->forall[i]) {
          reduce = h_lr_conflict(arena, table->forall[i], reduce);
          inadeq = true;
        }
        table->forall[i] = reduce;

        // check for shift/reduce conflict with other entries
        // NOTE: these are not recorded as HLR_CONFLICTs at this point

        if(!h_lrtable_row_empty(table, i))
          inadeq = true;
      }
    H_END_FOREACH

    if(inadeq)
      h_slist_push(table->inadeq, (void *)(uintptr_t)i);
  }

  return table;
}
