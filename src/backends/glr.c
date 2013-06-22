#include <assert.h>
#include "lr.h"


/* GLR compilation (LALR w/o failing on conflict) */

int h_glr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  int result = h_lalr_compile(mm__, parser, params);

  if(result == -1 && parser->backend_data) {
    // table is there, just has conflicts? nevermind, that's okay.
    result = 0;
  }

  return result;
}

void h_glr_free(HParser *parser)
{
  h_lalr_free(parser);
}


/* GLR driver */

HLREngine *fork_engine(const HLREngine *engine)
{
  HLREngine *eng2 = h_arena_malloc(engine->tarena, sizeof(HLREngine));
  eng2->table = engine->table;
  eng2->state = engine->state;
  eng2->input = engine->input;

  // shallow-copy the stack
  // this works because h_slist_push and h_slist_drop never modify
  // the underlying structure of HSlistNodes, only the head pointer.
  // in fact, this gives us prefix sharing for free.
  eng2->stack = h_arena_malloc(engine->tarena, sizeof(HSlist));
  *eng2->stack = *engine->stack;

  eng2->arena = engine->arena;
  eng2->tarena = engine->tarena;
  return eng2;
}

static void stow_engine(HSlist *engines, HLREngine *engine)
{
  // XXX switch to one engine per state, and do the merge here
  h_slist_push(engines, engine);
}

static const HLRAction *handle_conflict(HSlist *engines, const HLREngine *engine,
                                        const HSlist *branches)
{
  // there should be at least two conflicting actions
  assert(branches->head);
  assert(branches->head->next);     // this is just a consistency check

  // fork a new engine for all but the first action
  for(HSlistNode *x=branches->head->next; x; x=x->next) {
    HLRAction *act = x->elem; 
    HLREngine *eng = fork_engine(engine);

    // perform one step and add to list
    h_lrengine_step(eng, act);
    stow_engine(engines, eng);
  } 

  // return first action for use with original engine
  return branches->head->elem;
}

static HLREngine *handle_demerge(HSlist *engines, HLREngine *engine,
                                 const HLRAction *reduce)
{
  return engine; // XXX

  for(size_t i=0; i<reduce->production.length; i++) {
    // XXX if stack hits bottom, demerge
  }
  // XXX call step and stow on the newly-created engines
}

HParseResult *h_glr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  HLRTable *table = parser->backend_data;
  if(!table)
    return NULL;

  HArena *arena  = h_new_arena(mm__, 0);    // will hold the results
  HArena *tarena = h_new_arena(mm__, 0);    // tmp, deleted after parse

  HSlist *engines = h_slist_new(tarena);
  h_slist_push(engines, h_lrengine_new(arena, tarena, table, stream));

  HParseResult *result = NULL;
  while(result == NULL && !h_slist_empty(engines)) {
    for(HSlistNode **x = &engines->head; *x; ) {
      HLREngine *engine = (*x)->elem;

      // remove engine from list; it may come back in below
      *x = (*x)->next;    // advance x, removing the current element

      // drop those engines that have terminated
      if(!engine->run) {
        // check for parse success
        HParseResult *res = h_lrengine_result(engine);
        if(res)
          result = res;

        continue;
      }

      const HLRAction *action = h_lrengine_action(engine);

      // handle forks and demerges (~> spawn engines)
      if(action) {
        if(action->type == HLR_CONFLICT) {
          // fork engine on conflicts
          action = handle_conflict(engines, engine, action->branches);
        } else if(action->type == HLR_REDUCE) {
          // demerge as needed to ensure that stacks are deep enough
          engine = handle_demerge(engines, engine, action);
        }
      }

      h_lrengine_step(engine, action);
      stow_engine(engines, engine);
    }
  }

  if(!result)
    h_delete_arena(arena);
  h_delete_arena(tarena);
  return result;
}



HParserBackendVTable h__glr_backend_vtable = {
  .compile = h_glr_compile,
  .parse = h_glr_parse,
  .free = h_glr_free
};



// XXX TODO
// - implement engine merging
//   - triggered when two enter the same state
//   - old stacks (/engines?) saved
//   - new common suffix stack created
//   - when rewinding (during reduce), watch for empty stack -> demerge


// dummy!
int test_glr(void)
{
  HAllocator *mm__ = &system_allocator;

  /* 
     E -> E '+' E
        | 'd'
  */

  HParser *d = h_ch('d');
  HParser *E = h_indirect();
  HParser *E_ = h_choice(h_sequence(E, h_ch('+'), E, NULL), d, NULL);
  h_bind_indirect(E, E_);
  HParser *p = E;

  printf("\n==== G R A M M A R ====\n");
  HCFGrammar *g = h_cfgrammar_(mm__, h_desugar_augmented(mm__, p));
  if(g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }
  h_pprint_grammar(stdout, g, 0);

  printf("\n==== D F A ====\n");
  HLRDFA *dfa = h_lr0_dfa(g);
  if(dfa)
    h_pprint_lrdfa(stdout, g, dfa, 0);
  else
    fprintf(stderr, "h_lalr_dfa failed\n");

  printf("\n==== L R ( 0 )  T A B L E ====\n");
  HLRTable *table0 = h_lr0_table(g, dfa);
  if(table0)
    h_pprint_lrtable(stdout, g, table0, 0);
  else
    fprintf(stderr, "h_lr0_table failed\n");
  h_lrtable_free(table0);

  printf("\n==== L A L R  T A B L E ====\n");
  if(h_compile(p, PB_GLR, NULL)) {
    fprintf(stderr, "does not compile\n");
    return 2;
  }
  h_pprint_lrtable(stdout, g, (HLRTable *)p->backend_data, 0);

  printf("\n==== P A R S E  R E S U L T ====\n");
  HParseResult *res = h_parse(p, (uint8_t *)"d+d+d", 5);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
