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

HParseResult *h_glr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  HLRTable *table = parser->backend_data;
  if(!table)
    return NULL;

  HArena *arena  = h_new_arena(mm__, 0);    // will hold the results
  HArena *tarena = h_new_arena(mm__, 0);    // tmp, deleted after parse

  HSlist *engines = h_slist_new(tarena);
  h_slist_push(engines, h_lrengine_new(arena, tarena, table));

  HParseResult *result = NULL;
  while(result == NULL && !h_slist_empty(engines)) {
    for(HSlistNode **x = &engines->head; *x; ) {
      HLREngine *engine = (*x)->elem;

      const HLRAction *action = h_lrengine_action(engine, stream);
      // XXX handle conflicts -> fork engine
      bool running = h_lrengine_step(engine, action);

      if(running) {
        x = &(*x)->next;    // go to next
      } else {
        *x = (*x)->next;    // remove from list
        result = h_lrengine_result(engine);
      }
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




// dummy!
int test_glr(void)
{
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
  HCFGrammar *g = h_cfgrammar(&system_allocator, p);
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
