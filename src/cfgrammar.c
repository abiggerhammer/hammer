/* Context-free grammar representation and analysis */

#include "cfgrammar.h"
#include "allocator.h"
#include <assert.h>
#include <ctype.h>


HCFGrammar *h_cfgrammar_new(HAllocator *mm__)
{
  HCFGrammar *g = h_new(HCFGrammar, 1);
  assert(g != NULL);

  g->mm__   = mm__;
  g->arena  = h_new_arena(mm__, 0);     // default blocksize
  g->nts    = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  g->geneps = NULL;
  g->first  = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);
  g->follow = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);

  return g;
}

/* Frees the given grammar and associated data.
 * Does *not* free parsers' CFG forms as created by h_desugar.
 */
void h_cfgrammar_free(HCFGrammar *g)
{
  HAllocator *mm__ = g->mm__;
  h_delete_arena(g->arena);
  h_free(g);
}


// helpers
static void collect_nts(HCFGrammar *grammar, HCFChoice *symbol);
static void collect_geneps(HCFGrammar *grammar);


// XXX to be consolidated with glue.c when merged upstream
const HParsedToken *h_act_first(const HParseResult *p)
{
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[0];
}

/* Convert 'parser' into CFG representation by desugaring and compiling the set
 * of nonterminals.
 * A NULL return means we are unable to represent the parser as a CFG.
 */
HCFGrammar *h_cfgrammar(HAllocator* mm__, const HParser *parser)
{
  // convert parser to CFG form ("desugar").
  HCFChoice *desugared = h_desugar(mm__, parser);
  if(desugared == NULL)
    return NULL;  // -> backend not suitable for this parser

  HCFGrammar *g = h_cfgrammar_new(mm__);

  // recursively traverse the desugared form and collect all HCFChoices that
  // represent a nonterminal (type HCF_CHOICE or HCF_CHARSET).
  collect_nts(g, desugared);
  if(h_hashset_empty(g->nts)) {
    // desugared is a terminal. wrap it in a singleton HCF_CHOICE.
    HCFChoice *nt = h_new(HCFChoice, 1);
    nt->type = HCF_CHOICE;
    nt->seq = h_new(HCFSequence *, 2);
    nt->seq[0] = h_new(HCFSequence, 1);
    nt->seq[0]->items = h_new(HCFChoice *, 2);
    nt->seq[0]->items[0] = desugared;
    nt->seq[0]->items[1] = NULL;
    nt->seq[1] = NULL;
    nt->reshape = h_act_first;
    h_hashset_put(g->nts, nt);
    g->start = nt;
  } else {
    g->start = desugared;
  }

  // determine which nonterminals generate epsilon
  collect_geneps(g);

  return g;
}

/* Add all nonterminals reachable from symbol to grammar. */
static void collect_nts(HCFGrammar *grammar, HCFChoice *symbol)
{
  HCFSequence **s;  // for the rhs (sentential form) of a production
  HCFChoice **x;    // for a symbol in s

  if(h_hashset_present(grammar->nts, symbol))
    return;     // already visited, get out

  switch(symbol->type) {
  case HCF_CHAR:
  case HCF_END:
    break;  // it's a terminal symbol, nothing to do

  case HCF_CHARSET:
    break;  // NB charsets are considered terminal, too

  case HCF_CHOICE:
    // exploiting the fact that HHashSet is also a HHashTable to number the
    // nonterminals.
    // NB top-level (start) symbol gets 0.
    h_hashtable_put(grammar->nts, symbol,
                    (void *)(uintptr_t)grammar->nts->used);

    // each element s of symbol->seq (HCFSequence) represents the RHS of
    // a production. call self on all symbols (HCFChoice) in s.
    for(s = symbol->seq; *s != NULL; s++) {
      for(x = (*s)->items; *x != NULL; x++) {
        collect_nts(grammar, *x);
      }
    }
    break;

  default:  // should not be reachable
    assert_message(0, "unknown HCFChoice type");
  }
}


/* Does the given symbol derive the empty string (under g)? */
bool h_symbol_derives_epsilon(HCFGrammar *g, const HCFChoice *symbol)
{
  assert(g->geneps != NULL);

  switch(symbol->type) {
  case HCF_END:       // the end token doesn't count as empty
  case HCF_CHAR:
  case HCF_CHARSET:
    return false;
  default:  // HCF_CHOICE
    return h_hashset_present(g->geneps, symbol);
  }
}

/* Does the sentential form s derive the empty string? s NULL-terminated. */
bool h_sequence_derives_epsilon(HCFGrammar *g, HCFChoice **s)
{
  // return true iff all symbols in s derive epsilon
  for(; *s; s++) {
    if(!h_symbol_derives_epsilon(g, *s))
      return false;
  }
  return true;
}

/* Populate the geneps member of g; no-op if called multiple times. */
static void collect_geneps(HCFGrammar *g)
{
  if(g->geneps != NULL)
    return;

  g->geneps = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  assert(g->geneps != NULL);

  // iterate over the grammar's symbols, the elements of g->nts.
  // add any we can identify as deriving epsilon to g->geneps.
  // repeat until g->geneps no longer changes.
  size_t prevused;
  do {
    prevused = g->geneps->used;
    size_t i;
    HHashTableEntry *hte;
    for(i=0; i < g->nts->capacity; i++) {
      for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
        if(hte->key == NULL)
          continue;
        const HCFChoice *symbol = hte->key;
        assert(symbol->type == HCF_CHOICE);

        // this NT derives epsilon if any of its productions does.
        HCFSequence **p;
        for(p = symbol->seq; *p != NULL; p++) {
          if(h_sequence_derives_epsilon(g, (*p)->items)) {
            h_hashset_put(g->geneps, symbol);
            break;
          }
        }
      }
    }
  } while(g->geneps->used != prevused);
}


/* Compute first set of sentential form s. s NULL-terminated. */
HHashSet *h_first_sequence(HCFGrammar *g, HCFChoice **s);

/* Compute first set of symbol x. Memoized. */
HHashSet *h_first_symbol(HCFGrammar *g, const HCFChoice *x)
{
  HHashSet *ret;
  HCFSequence **p;
  uint8_t c;

  // memoize via g->first
  assert(g->first != NULL);
  ret = h_hashtable_get(g->first, x);
  if(ret != NULL)
    return ret;
  ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  assert(ret != NULL);
  h_hashtable_put(g->first, x, ret);

  switch(x->type) {
  case HCF_END:
    h_hashset_put(ret, (void *)end_token);
    break;
  case HCF_CHAR:
    h_hashset_put(ret, (void *)char_token(x->chr));
    break;
  case HCF_CHARSET:
    c=0;
    do {
      if(charset_isset(x->charset, c))
        h_hashset_put(ret, (void *)char_token(c));
    } while(c++ < 255);
    break;
  case HCF_CHOICE:
    // this is a nonterminal
    // return the union of the first sets of all productions
    for(p=x->seq; *p; ++p)
      h_hashset_put_all(ret, h_first_sequence(g, (*p)->items));
    break;
  default:  // should not be reached
    assert_message(0, "unknown HCFChoice type");
  }
  
  return ret;
}

HHashSet *h_first_sequence(HCFGrammar *g, HCFChoice **s)
{
  // the first set of the empty sequence is empty
  if(*s == NULL)
    return h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);

  // first(X tail) = first(X)                if X does not derive epsilon
  //               = first(X) u first(tail)  otherwise

  HCFChoice *x = s[0];
  HCFChoice **tail = s+1;

  HHashSet *first_x = h_first_symbol(g, x);
  if(h_symbol_derives_epsilon(g, x)) {
    // return the union of first(x) and first(tail)
    HHashSet *first_tail = h_first_sequence(g, tail);
    HHashSet *ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
    h_hashset_put_all(ret, first_x);
    h_hashset_put_all(ret, first_tail);
    return ret;
  } else {
    return first_x;
  }
}


/* Compute follow set of symbol x. Memoized. */
HHashSet *h_follow(HCFGrammar *g, const HCFChoice *x)
{
  // consider all occurances of X in g
  // the follow set of X is the union of:
  //   {$} if X is the start symbol
  //   given a production "A -> alpha X tail":
  //   if tail derives epsilon:
  //     first(tail) u follow(A)
  //   else:
  //     first(tail)

  HHashSet *ret;

  // memoize via g->follow
  assert(g->follow != NULL);
  ret = h_hashtable_get(g->follow, x);
  if(ret != NULL)
    return ret;
  ret = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  assert(ret != NULL);
  h_hashtable_put(g->follow, x, ret);

  // if X is the start symbol, the end token is in its follow set
  if(x == g->start)
    h_hashset_put(ret, (void *)end_token);

  // iterate over g->nts
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      const HCFChoice *a = hte->key;        // production's left-hand symbol
      assert(a->type == HCF_CHOICE);

      // iterate over the productions for A
      HCFSequence **p;
      for(p=a->seq; *p; p++) {
        HCFChoice **s = (*p)->items;        // production's right-hand side
        
        for(; *s; s++) {
          if(*s == x) { // occurance found
            HCFChoice **tail = s+1;

            h_hashset_put_all(ret, h_first_sequence(g, tail));
            if(h_sequence_derives_epsilon(g, tail))
              h_hashset_put_all(ret, h_follow(g, a));
          }
        }
      }
    }
  }

  return ret;
}


static void pprint_char(FILE *f, char c)
{
  switch(c) {
  case '"': fputs("\\\"", f); break;
  case '\\': fputs("\\\\", f); break;
  case '\b': fputs("\\b", f); break;
  case '\t': fputs("\\t", f); break;
  case '\n': fputs("\\n", f); break;
  case '\r': fputs("\\r", f); break;
  default:
    if(isprint(c)) {
      fputc(c, f);
    } else {
      fprintf(f, "\\x%.2X", c);
    }
  }
}

static void pprint_charset_char(FILE *f, char c)
{
  switch(c) {
  case '"': fputc(c, f); break;
  case '-': fputs("\\-", f); break;
  case ']': fputs("\\-", f); break;
  default:  pprint_char(f, c);
  }
}

static void pprint_charset(FILE *f, const HCharset cs)
{
  int i;

  fputc('[', f);
  for(i=0; i<256; i++) {
    if(charset_isset(cs, i))
      pprint_charset_char(f, i);

    // detect ranges
    if(i+2<256 && charset_isset(cs, i+1) && charset_isset(cs, i+2)) {
      fputc('-', f);
      for(; i<256 && charset_isset(cs, i); i++);
      i--;  // back to the last in range
      pprint_charset_char(f, i);
    }
  }
  fputc(']', f);
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

static HCFChoice **pprint_string(FILE *f, HCFChoice **x)
{
  fputc('"', f);
  for(; *x; x++) {
    if((*x)->type != HCF_CHAR)
      break;
    pprint_char(f, (*x)->chr);
  }
  fputc('"', f);
  return x;
}

static void pprint_symbol(FILE *f, const HCFGrammar *g, const HCFChoice *x)
{
  switch(x->type) {
  case HCF_CHAR:
    fputc('"', f);
    pprint_char(f, x->chr);
    fputc('"', f);
    break;
  case HCF_END:
    fputc('$', f);
    break;
  case HCF_CHARSET:
    pprint_charset(f, x->charset);
  default:
    fputs(nonterminal_name(g, x), f);
  }
}

static void pprint_sequence(FILE *f, const HCFGrammar *g, const HCFSequence *seq)
{
  HCFChoice **x = seq->items;

  if(*x == NULL) {  // the empty sequence
    fputs(" \"\"", f);
  } else {
    while(*x) {
      fputc(' ', f);      // separator

      if((*x)->type == HCF_CHAR) {
        // condense character strings
        x = pprint_string(f, x);
      } else {
        pprint_symbol(f, g, *x);
        x++;
      }
    }
  }

  fputc('\n', f);
}

static
void pprint_ntrules(FILE *f, const HCFGrammar *g, const HCFChoice *nt,
                    int indent, int len)
{
  int i;
  int column = indent + len;

  const char *name = nonterminal_name(g, nt);

  // print rule head (symbol name)
  for(i=0; i<indent; i++) fputc(' ', f);
  fputs(name, f);
  i += strlen(name);
  for(; i<column; i++) fputc(' ', f);
  fputs(" ->", f);

  assert(nt->type == HCF_CHOICE);
  HCFSequence **p = nt->seq;
  if(*p == NULL) return;          // shouldn't happen
  pprint_sequence(f, g, *p++);    // print first production on the same line
  for(; *p; p++) {                // print the rest below with "or" bars
    for(i=0; i<column; i++) fputc(' ', f);    // indent
    fputs("  |", f);
    pprint_sequence(f, g, *p);
  }
}

void h_pprint_grammar(FILE *file, const HCFGrammar *g, int indent)
{
  if(g->nts->used < 1)
    return;

  // determine maximum string length of symbol names
  int len;
  size_t s;
  for(len=1, s=26; s < g->nts->used; len++, s*=26); 

  // iterate over g->nts
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      const HCFChoice *a = hte->key;        // production's left-hand symbol
      assert(a->type == HCF_CHOICE);

      pprint_ntrules(file, g, a, indent, len);
    }
  }
}

void h_pprint_symbolset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent)
{
  int j;
  for(j=0; j<indent; j++) fputc(' ', file);

  fputc('{', file);

  // iterate over set
  size_t i;
  HHashTableEntry *hte;
  const HCFChoice *a = NULL;
  for(i=0; i < set->capacity; i++) {
    for(hte = &set->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      if(a != NULL) // we're not on the first element
          fputc(',', file);

      a = hte->key;        // production's left-hand symbol

      pprint_symbol(file, g, a);
    }
  }

  fputs("}\n", file);
}

void h_pprint_tokenset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent)
{
  int j;
  for(j=0; j<indent; j++) fputc(' ', file);

  fputc('[', file);

  // iterate over set
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < set->capacity; i++) {
    for(hte = &set->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      HCFToken a = (HCFToken)hte->key;
      
      if(a == end_token)
        fputc('$', file);
      else if(token_char(a) == '$')
        fputs("\\$", file);
      else
        pprint_char(file, token_char(a));
    }
  }

  fputs("]\n", file);
}
