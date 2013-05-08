/* Context-free grammar representation and analysis */

#include "internal.h"


typedef struct HCFGrammar_ {
  HCFChoice   *start;   // start symbol (nonterminal)
  HHashSet    *nts;     // HCFChoices, each representing the alternative
                        // productions for one nonterminal
  HHashSet    *geneps;  // set of NTs that can generate the empty string
  HHashTable  *first;   // memoized first sets of the grammar's symbols
  HHashTable  *follow;  // memoized follow sets of the grammar's NTs
  HArena      *arena;
  HAllocator  *mm__;
} HCFGrammar;

/* mapping input bytes or end to tokens
 * we want to use these, cast to void *, as elements in hashsets
 * therefore we must avoid 0 as a token value because NULL means "not in set".
 */
typedef uintptr_t HCFToken;
static inline HCFToken char_token(char c) { return (0x100 | c); }
static inline char token_char(HCFToken t) { return (0xFF & t); }
static const HCFToken end_token = 0x200;


/* Convert 'parser' into CFG representation by desugaring and compiling the set
 * of nonterminals.
 * A NULL return means we are unable to represent the parser as a CFG.
 */
HCFGrammar *h_cfgrammar(HAllocator* mm__, const HParser *parser);

/* Frees the given grammar and associated data.
 * Does *not* free parsers' CFG forms as created by h_desugar.
 */
void h_cfgrammar_free(HCFGrammar *g);

/* Does the given symbol derive the empty string (under g)? */
bool h_symbol_derives_epsilon(HCFGrammar *g, const HCFChoice *symbol);

/* Does the sentential form s derive the empty string? s NULL-terminated. */
bool h_sequence_derives_epsilon(HCFGrammar *g, HCFChoice **s);

/* Compute first set of sentential form s. s NULL-terminated. */
HHashSet *h_first_sequence(HCFGrammar *g, HCFChoice **s);

/* Compute first set of symbol x. Memoized. */
HHashSet *h_first_symbol(HCFGrammar *g, const HCFChoice *x);

/* Compute follow set of symbol x. Memoized. */
HHashSet *h_follow(HCFGrammar *g, const HCFChoice *x);


/* Pretty-printers for grammars and associated data. */
void h_pprint_grammar(FILE *file, const HCFGrammar *g, int indent);
void h_pprint_symbolset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent);
void h_pprint_tokenset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent);
