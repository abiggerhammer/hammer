#include "lr.h"



/* GLR driver */

HParseResult *h_glr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
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



HParserBackendVTable h__glr_backend_vtable = {
  .compile = h_lalr_compile,
  .parse = h_glr_parse,
  .free = h_lalr_free
};




// dummy!
int test_glr(void)
{
  /* 
     E -> E '-' T
        | T
     T -> '(' E ')'
        | 'n'               -- also try [0-9] for the charset paths
  */

  HParser *n = h_ch('n');
  HParser *E = h_indirect();
  HParser *T = h_choice(h_sequence(h_ch('('), E, h_ch(')'), NULL), n, NULL);
  HParser *E_ = h_choice(h_sequence(E, h_ch('-'), T, NULL), T, NULL);
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
  HParseResult *res = h_parse(p, (uint8_t *)"n-(n-((n)))-n", 13);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
