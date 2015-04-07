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

void h_pprint_gfexpr(FILE *file, const HCFGrammar *g, HCFSequence *seq) {
  HCFChoice **x = seq->items;
  
  if (*x == NULL) { // empty sequence
    fprintf(file, "1\n");
  } else {
    while (*x) {
      if (x != seq->items) {
	fprintf(file, " + ");
      }
      // consume items
      // if a string, 
      //    count its length
      //    output t^length

      if ((*x)->type == HCF_CHAR) {
	uint32_t count = 0;
	for(; *x; x++, count++) {
	  if ((*x)->type != HCF_CHAR) {
	    break;
	  }
	}
	fprintf(file, "t^%d", count);
      } else {
	uint32_t count=0, n, i=0;
	switch((*x)->type) {
	case HCF_CHAR:
	  // should not be possible
	  break;
	case HCF_END:
	  // does not generate any output symbols: value 0
	  break;
	case HCF_CHARSET:
	  for(i=0; i<256; i++) {
	    if (charset_isset((*x)->charset, i)) {
	      count++;
	    }
	  }
	  fprintf(file, "%d*t", count);
	  break;
	default:
	  n = (uint8_t)(uintptr_t)h_hashtable_get(g->nts, x);

	  fprintf(file, "%c(t)", 'A'+n);
	}
        x++;
      }
    }
  }
}

      
void h_pprint_gfeqns_NOTUSED(FILE *file, const HCFGrammar *g) {
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
      const HCFChoice *lhs = hte->key;        // production's left-hand symbol
      assert(lhs->type == HCF_CHOICE);

      uint8_t n = (uint8_t)(uintptr_t)h_hashtable_get(g->nts, lhs);
      fprintf(file, "%c(t) = ", 'A'+n);

      HCFSequence **p = lhs->seq;
      if (*p == NULL) {
	return;          // shouldn't happen
      }

      h_pprint_gfexpr(file, g, *p);
      for(; *p; p++) {
	fprintf(file, "\t");
	h_pprint_gfexpr(file, g, *p);      
	fprintf(file, "\n");
      }
    }
  }
}


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




int main(int argc, char **argv)
{
  HAllocator *mm__ = &system_allocator;

  HParser *n = h_ch('n');
  HParser *E = h_indirect();
  HParser *T = h_choice(h_sequence(h_ch('('), E, h_ch(')'), NULL), n, NULL);
  HParser *E_ = h_choice(h_sequence(E, h_ch('-'), T, NULL), T, NULL);
  h_bind_indirect(E, E_);
  HParser *p = E;

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
