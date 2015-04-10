// Intention: read in a parser, generate the system of equations for its
// generating functions
//

#include <inttypes.h>
#include "../src/backends/contextfree.h"
#include "../src/backends/lr.h"
#include "grammar.h"
#include <stdio.h>


HAllocator *mm__;

HParser* cfExample() {
  HParser *n = h_ch('n');
  HParser *E = h_indirect();
  HParser *T = h_choice(h_sequence(h_ch('('), E, h_ch(')'), NULL), n, NULL);
  HParser *E_ = h_choice(h_sequence(E, h_ch('-'), T, NULL), T, NULL);
  h_bind_indirect(E, E_);
  return E;
}

// The tie knot parsers below would work better if we could patch the gen.function
// code above to allow user specification of non-default byte string "lengths",
// so that U symbols don't contribute with factors of t to the gen. function.
//
// Alternatively: use multivariate generating functions to spit out different
// variables for different terminals. This gets really messy with bigger alphabets.

HParser* finkmao() {
  HParser *L = h_ch('L');
  HParser *R = h_ch('R');
  HParser *C = h_ch('C');
  HParser *U = h_ch('U');
  HParser *Lnext = h_indirect();
  HParser *Rnext = h_indirect();
  HParser *Cnext = h_indirect();
  HParser *L_ = h_choice(h_sequence(R, Rnext, NULL),
			 h_sequence(C, Cnext, NULL),
			 h_sequence(R, C, U, NULL), NULL);
  HParser *R_ = h_choice(h_sequence(L, Lnext, NULL),
			 h_sequence(C, Cnext, NULL),
			 h_sequence(L, C, U, NULL), NULL);
  HParser *C_ = h_choice(h_sequence(R, Rnext, NULL),
			 h_sequence(L, Lnext, NULL), NULL);
  h_bind_indirect(Lnext, L_);
  h_bind_indirect(Rnext, R_);
  h_bind_indirect(Cnext, C_);
  HParser *tie = h_sequence(L, Lnext, NULL);

  h_desugar_augmented(mm__, tie);

  L->desugared->user_data = "L";
  R->desugared->user_data = "R";
  C->desugared->user_data = "C";
  Lnext->desugared->user_data = "Ln";
  Rnext->desugared->user_data = "Rn";
  Cnext->desugared->user_data = "Cn";
  tie->desugared->user_data = "tie";
  U->desugared->user_data = "0U";
  
  return tie;
}

HParser* finkmaoTW() {
  HParser *T = h_ch('T');
  HParser *W = h_ch('W');
  HParser *U = h_ch('U');
  HParser *prefix = h_choice(T, W, h_epsilon_p(),
			     NULL);
  HParser *pair = h_choice(h_sequence(T, T, NULL),
			   h_sequence(W, T, NULL),
			   h_sequence(T, W, NULL),
			   h_sequence(W, W, NULL), NULL);
  HParser *tuck = h_choice(h_sequence(T, T, U, NULL),
			   h_sequence(W, W, U, NULL),
			   NULL);
  HParser *pairstar = h_indirect();
  HParser *pstar_ = h_choice(h_sequence(pair, pairstar, NULL),
			     h_epsilon_p(),
			      NULL);
  h_bind_indirect(pairstar, pstar_);

  HParser* tie = h_sequence(prefix, pairstar, tuck, NULL);
  h_desugar_augmented(mm__, tie);

  
  T->desugared->user_data = "T";
  W->desugared->user_data = "W";
  U->desugared->user_data = "0U";
  prefix->desugared->user_data = "prefix";
  pair->desugared->user_data = "pair";
  tuck->desugared->user_data = "tuck";
  pstar_->desugared->user_data = "pairstar";
  tie->desugared->user_data = "tie";
  
  return tie;
}

HParser* depth1TW() {
  HParser *T = h_ch('T');
  HParser *W = h_ch('W');
  HParser *U = h_ch('U');
  HParser *prefix = h_choice(T, W, h_epsilon_p(), NULL);
  HParser *pair = h_choice(h_sequence(T, T, NULL),
			   h_sequence(W, T, NULL),
			   h_sequence(T, W, NULL),
			   h_sequence(W, W, NULL), NULL);
  HParser *tuck = h_choice(h_sequence(T, T, U, NULL),
			   h_sequence(W, W, U, NULL),
			   NULL);
  HParser *tuckpairstar = h_indirect();
  HParser *tpstar_ = h_choice(h_sequence(pair, tuckpairstar, NULL),
			      h_sequence(tuck, tuckpairstar, NULL),
			      h_epsilon_p(),
			      NULL);
  h_bind_indirect(tuckpairstar, tpstar_);
  HParser *tie = h_choice(h_sequence(prefix, tuckpairstar, tuck, NULL), NULL);

  h_desugar_augmented(mm__, tie);
  
  T->desugared->user_data = "T";
  W->desugared->user_data = "W";
  U->desugared->user_data = "0U";
  prefix->desugared->user_data = "prefix";
  pair->desugared->user_data = "pair";
  tuck->desugared->user_data = "tuck";
  tpstar_->desugared->user_data = "tuckpairstar";
  tie->desugared->user_data = "tie";

  return tie;
}

HParser* depth1() {
  HParser *L = h_ch('L');
  HParser *R = h_ch('R');
  HParser *C = h_ch('C');
  HParser *U = h_ch('U');
  HParser *lastR = h_indirect();
  HParser *lastL = h_indirect();
  HParser *lastC = h_indirect();
  HParser *R_ = h_choice(h_sequence(L, R, lastR, NULL),
			 h_sequence(C, R, lastR, NULL),
			 h_sequence(L, C, lastC, NULL),
			 h_sequence(L, C, U, lastC, NULL),
			 h_sequence(L, C, U, NULL),
			 h_sequence(C, L, lastL, NULL),
			 h_sequence(C, L, U, lastL, NULL),
			 h_sequence(C, L, U, NULL),
			 NULL);
  HParser *L_ = h_choice(h_sequence(R, L, lastR, NULL),
			 h_sequence(C, L, lastR, NULL),
			 h_sequence(R, C, lastC, NULL),
			 h_sequence(R, C, U, lastC, NULL),
			 h_sequence(R, C, U, NULL),
			 h_sequence(C, R, lastR, NULL),
			 h_sequence(C, R, U, lastR, NULL),
			 h_sequence(C, R, U, NULL),
			 NULL);
  HParser *C_ = h_choice(h_sequence(L, C, lastR, NULL),
			 h_sequence(R, C, lastR, NULL),
			 h_sequence(L, R, lastR, NULL),
			 h_sequence(L, R, U, lastR, NULL),
			 h_sequence(L, R, U, NULL),
			 h_sequence(R, L, lastL, NULL),
			 h_sequence(R, L, U, lastL, NULL),
			 h_sequence(R, L, U, NULL),
			 NULL);
  h_bind_indirect(lastR, R_);
  h_bind_indirect(lastL, L_);
  h_bind_indirect(lastC, C_);
  HParser* tie = h_choice(h_sequence(L, lastL, NULL),
		  h_sequence(R, lastR, NULL),
		  h_sequence(C, lastC, NULL),
		  NULL);

  h_desugar_augmented(mm__, tie);

  L->desugared->user_data = "L";
  R->desugared->user_data = "R";
  C->desugared->user_data = "C";
  U->desugared->user_data = "0U";
  lastL ->desugared->user_data = "Ln";
  lastR->desugared->user_data = "Rn";
  lastC->desugared->user_data = "Cn";
  tie->desugared->user_data = "tie";

  return tie;
}

HParser* depthNTW() {
  HParser *T = h_ch('T');
  HParser *W = h_ch('W');
  HParser *U = h_ch('U');
  HParser *prefix = h_choice(T, W, h_epsilon_p(), NULL);
  HParser *pair = h_choice(h_sequence(T, T, NULL),
			   h_sequence(W, T, NULL),
			   h_sequence(T, W, NULL),
			   h_sequence(W, W, NULL), NULL);
  HParser *tstart = h_indirect();
  HParser *tw0 = h_indirect();
  HParser *tw1 = h_indirect();
  HParser *tw2 = h_indirect();
  HParser *wstart = h_indirect();
  HParser *wt0 = h_indirect();
  HParser *wt1 = h_indirect();
  HParser *wt2 = h_indirect();
  
  HParser *T_ = h_choice(h_sequence(T, T, tw2, U, NULL),
			 h_sequence(T, W, tw0, U, NULL),
			 NULL);
  HParser *tw0_ = h_choice(h_sequence(T, T, tw2, U, NULL),
			   h_sequence(T, W, tw0, U, NULL),
			   h_sequence(W, T, tw0, U, NULL),
			   h_sequence(W, W, tw1, U, NULL),
			   h_sequence(tstart, tw2, U, NULL),
			   h_sequence(wstart, tw1, U, NULL),
			   NULL);
  HParser *tw1_ = h_choice(h_sequence(T, T, tw0, U, NULL),
			   h_sequence(T, W, tw1, U, NULL),
			   h_sequence(W, T, tw1, U, NULL),
			   h_sequence(W, W, tw2, U, NULL),
			   h_sequence(tstart, tw0, U, NULL),
			   h_sequence(wstart, tw2, U, NULL),
			   NULL);
  HParser *tw2_ = h_choice(h_sequence(T, T, tw1, U, NULL),
			   h_sequence(T, W, tw2, U, NULL),
			   h_sequence(W, T, tw2, U, NULL),
			   h_sequence(W, W, tw0, U, NULL),
			   h_sequence(tstart, tw1, U, NULL),
			   h_sequence(wstart, tw0, U, NULL),
			   h_epsilon_p(),
			   NULL);
  
  HParser *W_ = h_choice(h_sequence(W, W, wt2, U, NULL),
			 h_sequence(W, T, wt0, U, NULL),
			 NULL);
  HParser *wt0_ = h_choice(h_sequence(W, W, wt2, U, NULL),
			   h_sequence(W, T, wt0, U, NULL),
			   h_sequence(T, W, wt0, U, NULL),
			   h_sequence(T, T, wt1, U, NULL),
			   h_sequence(wstart, wt2, U, NULL),
			   h_sequence(tstart, wt1, U, NULL),
			   NULL);
  HParser *wt1_ = h_choice(h_sequence(W, W, wt0, U, NULL),
			   h_sequence(W, T, wt1, U, NULL),
			   h_sequence(T, W, wt1, U, NULL),
			   h_sequence(T, T, wt2, U, NULL),
			   h_sequence(wstart, wt0, U, NULL),
			   h_sequence(tstart, wt2, U, NULL),
			   NULL);
  HParser *wt2_ = h_choice(h_sequence(W, W, wt1, U, NULL),
			   h_sequence(W, T, wt2, U, NULL),
			   h_sequence(T, W, wt2, U, NULL),
			   h_sequence(T, T, wt0, U, NULL),
			   h_sequence(wstart, wt1, U, NULL),
			   h_sequence(tstart, wt0, U, NULL),
			   h_epsilon_p(),
			   NULL);

  h_bind_indirect(tstart, T_);
  h_bind_indirect(tw0, tw0_);
  h_bind_indirect(tw1, tw1_);
  h_bind_indirect(tw2, tw2_);
  h_bind_indirect(wstart, W_);
  h_bind_indirect(wt0, wt0_);
  h_bind_indirect(wt1, wt1_);
  h_bind_indirect(wt2, wt2_);
  HParser *tuck = h_choice(tstart, wstart, NULL);

  HParser *tuckpairstar = h_indirect();
  HParser *tpstar_ = h_choice(h_sequence(pair, tuckpairstar, NULL),
			      h_sequence(tuck, tuckpairstar, NULL),
			      h_epsilon_p(),
			      NULL);
  h_bind_indirect(tuckpairstar, tpstar_);
			      
  HParser *tie = h_choice(h_sequence(prefix, tuckpairstar, tuck, NULL), NULL);

  h_desugar_augmented(mm__, tie);

  T->desugared->user_data = "T";
  W->desugared->user_data = "W";
  U->desugared->user_data = "0U";
  prefix->desugared->user_data = "prefix";
  pair->desugared->user_data = "pair";
  tuck->desugared->user_data = "tuck";
  tpstar_->desugared->user_data = "tuckpairstar";
  tie->desugared->user_data = "tie";

  return tie;
}


int main(int argc, char **argv) {
  mm__ = &system_allocator;

  HParser *p = finkmao();
  HCFGrammar *g = h_cfgrammar_(mm__, h_desugar_augmented(mm__, p));
  if (g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }
  printf("\n==== Generating functions ====\n");
  h_pprint_gfeqns(stdout, g);

  printf("\n==== Grammar ====\n");
  h_pprint_grammar(stdout, g, 0);
}
