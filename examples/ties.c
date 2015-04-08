//
// Created by Mikael Vejdemo Johansson on 4/7/15.
//
// Intention: read in a parser, generate the system of equations for its
// generating functions
//

#include <inttypes.h>
#include "../src/backends/contextfree.h"
#include "../src/backends/lr.h"
#include <stdio.h>

static const char *nonterminal_name(const HCFGrammar *g, const HCFChoice *nt)
{
  static char buf[16] = {0}; // 14 characters in base 26 are enough for 64 bits

  // find nt's number in g
  size_t n = (uintptr_t)h_hashtable_get(g->nts, nt);

  // NB the start symbol (number 0) is always "A".
  int i;
  for(i=14; i>=0 && (n>0 || i==14); i--) {
    buf[i] = 'A' + n%26;
    n = n/26;   // shift one digit
  }

  return buf+i+1;
}



void readsequence(FILE *file, uint32_t *count, uint32_t *length,
		  const HCFGrammar *g, const HCFSequence *seq) {
  // tally up numbers of choices, and lengths of emitted strings.
  // Immediately emit any nonterminals encountered.
  HCFChoice** x = seq->items;
  
  if (*x == NULL) {
    return;
  } else {
    fprintf(file, "1");
    HCharset cs;
    unsigned int i, cscount=0;
    for(; *x; x++) {
      switch((*x)->type) {
      case HCF_CHAR:
	(*length)++;
	break;
      case HCF_END:
	break;
      case HCF_CHARSET:
	cs = (*x)->charset;
	for(i=0; i<256; i++) {
	  if (charset_isset(cs, i)) {
	    cscount++;
	  }
	}
	*count *= cscount;
	break;
      default: // HCF_CHOICE, non-terminal symbol
	fprintf(file, "*%s(t)", nonterminal_name(g, *x));
	break;
      }
    }
  }
}

// For each nt in g->nts
//     For each choice in nt->key->seq
//          For all elements in sequence
//              Accumulate counts 
//              Accumulate string lengths
//              Emit count*t^length
void h_pprint_gfeqns(FILE *file, const HCFGrammar *g) {
  if (g->nts->used < 1) {
    return;
  }

  // determine maximum string length of symbol names
  int len;
  size_t s;
  for(len=1, s=26; s < g->nts->used; len++, s*=26); 

  // iterate over g->nts
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }

      const HCFChoice *nt = hte->key;
      fprintf(file, "%s(t) = ", nonterminal_name(g, nt));

      
      for(HCFSequence **seq = nt->seq; *seq; seq++) {
	if (seq != nt->seq) {
	  fprintf(file, " + ");
	}
	uint32_t count=1, length=0;
	readsequence(file, &count, &length, g, *seq);
	if(count == 1) {
	  if(length == 1) {
	    fprintf(file, "*t");
	  }
	  if(length > 1) {
	    fprintf(file, "*t^%d", length);
	  }
	} else if(count > 1) {
	  if(length == 0) {
	    fprintf(file, "*%d", count);
	  }
	  if(length == 1) {
	    fprintf(file, "*%d*t", count);
	  }
	  if (length > 1) {
	    fprintf(file, "*%d*t^%d", count, length);
	  } 
	}
      }

      fprintf(file, "\n");
    }
  }
}

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
  HParser *tie = h_choice(h_sequence(L, Lnext, NULL), NULL);
  return tie;
}

HParser* finkmaoTW() {
  HParser *T = h_ch('T');
  HParser *W = h_ch('W');
  HParser *U = h_ch('U');
  HParser *prefix = h_choice(T, W, h_epsilon_p(), NULL);
  HParser *pair = h_repeat_n(h_choice(T, W, NULL), 2);
  HParser *tuck = h_choice(h_sequence(T, T, U, NULL),
			   h_sequence(W, W, U, NULL),
			   NULL);
  HParser *pairstar = h_indirect();
  HParser *pstar_ = h_choice(h_sequence(pair, pairstar, NULL),
			      h_epsilon_p(),
			      NULL);
  h_bind_indirect(pairstar, pstar_);
  return h_choice(h_sequence(prefix, pairstar, tuck, NULL), NULL);
}

HParser* depth1TW() {
  HParser *T = h_ch('T');
  HParser *W = h_ch('W');
  HParser *U = h_ch('U');
  HParser *prefix = h_choice(T, W, h_epsilon_p(), NULL);
  HParser *pair = h_repeat_n(h_choice(T, W, NULL), 2);
  HParser *tuck = h_choice(h_sequence(T, T, U, NULL),
			   h_sequence(W, W, U, NULL),
			   NULL);
  HParser *tuckpairstar = h_indirect();
  HParser *tpstar_ = h_choice(h_sequence(pair, tuckpairstar, NULL),
			      h_sequence(tuck, tuckpairstar, NULL),
			      h_epsilon_p(),
			      NULL);
  h_bind_indirect(tuckpairstar, tpstar_);
  return h_choice(h_sequence(prefix, tuckpairstar, tuck, NULL), NULL);
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
  return h_choice(h_sequence(L, lastL, NULL),
		  h_sequence(R, lastR, NULL),
		  h_sequence(C, lastC, NULL),
		  NULL);
}

HParser* depthNTW() {
  HParser *T = h_ch('T');
  HParser *W = h_ch('W');
  HParser *U = h_ch('U');
  HParser *prefix = h_choice(T, W, h_epsilon_p(), NULL);
  HParser *pair = h_repeat_n(h_choice(T, W, NULL), 2);
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
			      
  return h_choice(h_sequence(prefix, tuckpairstar, tuck, NULL), NULL);
}


int main(int argc, char **argv)
{
  HAllocator *mm__ = &system_allocator;


  HCFGrammar *g = h_cfgrammar_(mm__, h_desugar_augmented(mm__, finkmaoTW()));
  if (g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }
  printf("\n==== Generating functions ====\n");
  h_pprint_gfeqns(stdout, g);

  printf("\n==== Grammar ====\n");
  h_pprint_grammar(stdout, g, 0);
}
