// Generates a system of equations for generating functions from a grammar.
//
// (c) 2015 Mikael Vejdemo-Johansson <mikael@johanssons.org>
//

// If a desugared parser has user_data set, the generating function systems will try
// to interpret it as a string:
//
// If this string for an h_ch starts with the character 0, then that character
// will have weight 0 in the generating function.
//
// Use the remaining string to set the preferred name of that parser in the
// generating function.
//

#include <inttypes.h>
#include "../src/backends/contextfree.h"
#include "../src/backends/lr.h"
#include "grammar.h"
#include <stdio.h>

const char *nonterminal_name(const HCFGrammar *g, const HCFChoice *nt) {
  // if user_data exists and is printable:
  if(nt->user_data != NULL && *(char*)(nt->user_data) > ' ' && *(char*)(nt->user_data) < 127) {
    if(*(char*)(nt->user_data) != '0') {
      // user_data is a non-empty string
      return nt->user_data;
    } else {
      return nt->user_data+1;
    }
  }
  
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
  
  fprintf(file, "1");
  if (*x == NULL) {
    // empty sequence
    // GF is 1
    return;
  } else {
    char has_user_data = (*x)->user_data != NULL && *(char*)(*x)->user_data != 0;
    HCharset cs;
    unsigned int i, cscount=0;
    for(; *x; x++) {
      switch((*x)->type) {
      case HCF_CHAR:
	if(!(has_user_data && *(char*)(*x)->user_data == '0')) {
	  (*length)++;
	}
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
	fprintf(file, "*%s", nonterminal_name(g, *x));
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

  // emit the SageMath ring init string
  // iterate over g->nts, output symbols
  size_t i;
  HHashTableEntry *hte;  
  fprintf(file, "ring.<t");
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      const HCFChoice *nt = hte->key;
      fprintf(file, ",");
      
      fprintf(file, "%s", nonterminal_name(g, nt));
    }
  }
  fprintf(file, "> = QQ[]\n");
      
  
  // iterate over g->nts
  // emit a Sage ideal definition
  int j=0;
  fprintf(file, "ID = ring.ideal(");
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }

      if(j>0) {
	fprintf(file, ",");
      }
      j++;
      
      const HCFChoice *nt = hte->key;
      const char *ntn = nonterminal_name(g, nt);
      if(*ntn == 0) {
	continue;
      }
      fprintf(file, "%s - (", ntn);

      
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

      fprintf(file, ")");
    }
  }
  fprintf(file, ")\n");
}
