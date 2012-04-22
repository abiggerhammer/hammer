#include <glib.h>
#include <stdint.h>

/* The state of the parser.
 *
 * Members:
 *   input - the entire string being parsed
 *   index - current position in input
 *   length - size of input
 * THE FOLLOWING DESCRIBES HOW JSPARSE DOES IT. OUR MILEAGE MAY VARY.
 *   cache - a hash table describing the state of the parse, including partial parse
 *           results. It's actually a hash table of [parser_id, hash_table[index, result]],
 *           where the parser id is incremented as the parse goes along (parsers that have
 *           already been applied once don't get a new parser_id ... but the global variable
 *           still increments? not sure why that is, need to debug some), and the locations
 *           at which it's been applied are memoized.
 *
 */
typedef struct {
  const uint8_t *input;
  size_t index;
  size_t length;
  GHashTable *cache; 
} parse_state;

typedef struct {
  const uint8_t *remaining;
  const uint8_t *matched;
  const GSequence *ast;
} result;

typedef result*(*parser)(parse_state*);

parser *token(const uint8_t *s);
parser *ch(const uint8_t c);
parser *range(const uint8_t lower, const uint8_t upper);
parser *whitespace(parser *p);
//parser *action(parser *p, /* fptr to action on AST */);
parser *join_action(parser *p, const uint8_t *sep);
parser *left_faction_action(parser *p);
parser *negate(parser *p);
parser *end_p();
parser *nothing_p();
parser *sequence(parser **p_array);
parser *choice(parser **p_array);
parser *butnot(parser *p1, parser *p2);
parser *difference(parser *p1, parser *p2);
parser *xor(parser *p1, parser *p2);
parser *repeat0(parser *p);
parser *repeat1(parser *p);
parser *repeat_n(parser *p, const size_t n);
parser *optional(parser *p);
parser *expect(parser *p);
parser *chain(parser *p1, parser *p2, parser *p3);
parser *chainl(parser *p1, parser *p2);
parser *list(parser *p1, parser *p2);
parser *epsilon_p();
//parser *semantic(/* fptr to nullary function? */);
parser *and(parser *p);
parser *not(parser *p);



