#include <assert.h>
#include "../internal.h"
#include "../cfgrammar.h"
#include "../parsers/parser_internal.h"



void h_lalr_free(HParser *parser)
{
  // XXX free data structures
  parser->backend_data = NULL;
  parser->backend = PB_PACKRAT;
}


/* LALR table generation */

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  return -1;
}


/* LR driver */

HParseResult *h_lr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  return NULL;
}




HParserBackendVTable h__lalr_backend_vtable = {
  .compile = h_lalr_compile,
  .parse = h_lr_parse,
  .free = h_lalr_free
};




// dummy!
int test_lalr(void)
{
  /* for k=2:

     S -> A | B
     A -> X Y a
     B -> Y b
     X -> x | ''
     Y -> y         -- for k=3 use "yy"
  */

  // XXX make LALR example
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
  // print states of the LR(0) automaton
  // print LALR(1) table

  if(h_compile(p, PB_LALR, NULL)) {
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
