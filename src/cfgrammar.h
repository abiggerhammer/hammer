/* Context-free grammar representation and analysis */

#include "internal.h"


typedef struct HCFGrammar_ {
  HCFChoice   *start;   // start symbol (nonterminal)
  HHashSet    *nts;     // HCFChoices, each representing the alternative
                        // productions for one nonterminal
  HHashSet    *geneps;  // set of NTs that can generate the empty string
  HHashTable  **first;  // memoized first sets of the grammar's symbols
  HHashTable  **follow; // memoized follow sets of the grammar's NTs
  size_t      kmax;     // maximum lookahead depth allocated
  HArena      *arena;
  HAllocator  *mm__;

  // constant set containing only the empty string.
  // this is only a member of HCFGrammar because it needs a pointer to arena.
  const struct HStringMap_ *singleton_epsilon;
} HCFGrammar;


/* Representing input characters (bytes) in HHashTables.
 * To use these as keys, we must avoid 0 as because NULL means "not set".
 */
typedef uintptr_t HCharKey;
static inline HCharKey char_key(uint8_t c) { return (0x100 | c); }
static inline uint8_t key_char(HCharKey k) { return (0xFF & k); }

/* Mapping strings of input tokens to arbitrary values (or serving as a set).
 * Common prefixes are folded into a tree of HHashTables, branches labeled with
 * input tokens.
 * Each path through the tree represents the string along its branches.
 */
typedef struct HStringMap_ {
  void *epsilon_branch;         // points to leaf value
  void *end_branch;             // points to leaf value
  HHashTable *char_branches;    // maps to inner nodes (HStringMaps)
  HArena *arena;
} HStringMap;

HStringMap *h_stringmap_new(HArena *a);
HStringMap *h_stringmap_copy(HArena *a, const HStringMap *m);
void h_stringmap_put_end(HStringMap *m, void *v);
void h_stringmap_put_epsilon(HStringMap *m, void *v);
void h_stringmap_put_after(HStringMap *m, uint8_t c, HStringMap *ends);
void h_stringmap_put_char(HStringMap *m, uint8_t c, void *v);
void h_stringmap_update(HStringMap *m, const HStringMap *n);
void h_stringmap_replace(HStringMap *m, void *old, void *new);
void *h_stringmap_get(const HStringMap *m, const uint8_t *str, size_t n, bool end);
void *h_stringmap_get_lookahead(const HStringMap *m, HInputStream lookahead);
bool h_stringmap_present(const HStringMap *m, const uint8_t *str, size_t n, bool end);
bool h_stringmap_present_epsilon(const HStringMap *m);
bool h_stringmap_empty(const HStringMap *m);

static inline HStringMap *h_stringmap_get_char(const HStringMap *m, const uint8_t c)
 { return h_hashtable_get(m->char_branches, (void *)char_key(c)); }

// dummy return value used by h_stringmap_get_lookahead when out of input
#define NEED_INPUT ((void *)-1)


/* Convert 'parser' into CFG representation by desugaring and compiling the set
 * of nonterminals.
 * A NULL return means we are unable to represent the parser as a CFG.
 */
HCFGrammar *h_cfgrammar(HAllocator* mm__, const HParser *parser);
HCFGrammar *h_cfgrammar_(HAllocator* mm__, HCFChoice *start);

HCFGrammar *h_cfgrammar_new(HAllocator *mm__);

/* Frees the given grammar and associated data.
 * Does *not* free parsers' CFG forms as created by h_desugar.
 */
void h_cfgrammar_free(HCFGrammar *g);

/* Does the given symbol derive the empty string (under g)? */
bool h_derives_epsilon(HCFGrammar *g, const HCFChoice *symbol);

/* Does the sentential form s derive the empty string? s NULL-terminated. */
bool h_derives_epsilon_seq(HCFGrammar *g, HCFChoice **s);

/* Compute first_k set of symbol x. Memoized. */
const HStringMap *h_first(size_t k, HCFGrammar *g, const HCFChoice *x);

/* Compute first_k set of sentential form s. s NULL-terminated. */
const HStringMap *h_first_seq(size_t k, HCFGrammar *g, HCFChoice **s);

/* Compute follow_k set of symbol x. Memoized. */
const HStringMap *h_follow(size_t k, HCFGrammar *g, const HCFChoice *x);

/* Compute the predict_k set of production "A -> rhs".
 * Always returns a newly-allocated HStringMap.
 */
HStringMap *h_predict(size_t k, HCFGrammar *g,
                        const HCFChoice *A, const HCFSequence *rhs);


/* Pretty-printers for grammars and associated data. */
void h_pprint_grammar(FILE *file, const HCFGrammar *g, int indent);
void h_pprint_sequence(FILE *f, const HCFGrammar *g, const HCFSequence *seq);
void h_pprint_symbol(FILE *f, const HCFGrammar *g, const HCFChoice *x);
void h_pprint_symbolset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent);
void h_pprint_stringset(FILE *file, const HStringMap *set, int indent);
void h_pprint_stringmap(FILE *file, char sep,
                        void (*valprint)(FILE *f, void *env, void *val), void *env,
                        const HStringMap *map);
void h_pprint_char(FILE *file, uint8_t c);
