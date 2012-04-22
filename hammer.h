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

result* (*token(const uint8_t *s))(parse_state *ps);
result* (*ch(const uint8_t c))(parse_state *ps);
result* (*range(const uint8_t lower, const uint8_t upper))(parse_state *ps);
result* (*whitespace(result *(*p1)(parse_state *ps1)))(parse_state *ps);
//result (*action(result *(*p1)(parse_state *ps1), /* fptr to action on AST */))(parse_state *ps);
result* (*join_action(result *(*p1)(parse_state *ps1), const uint8_t *sep))(parse_state *ps);
result* (*left_factor_action(result *(*p1)(parse_state *ps1)))(parse_state *ps);
result* (*negate(result *(*p1)(parse_state *ps1)))(parse_state *ps);
result* end_p(parse_state *ps);
result* nothing_p(parse_state *ps);
result* (*sequence(result *(*(*p1)(parse_state *ps1))))(parse_state *ps);
result* (*choice(result *(*(*p1)(parse_state *ps1))))(parse_state *ps);
result* (*butnot(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps);
result* (*difference(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps);
result* (*xor(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps);
result* (*repeat0(result *(*p1)(parse_state *ps1)))(parse_state *ps);
result* (*repeat1(result *(*p1)(parse_state *ps1)))(parse_state *ps);
result* (*repeatN(result *(*p1)(parse_state *ps1), const size_t n))(parse_state *ps);
result* (*optional(result *(*p1)(parse_state *ps1)))(parse_state *ps);
void (*expect(result *(*p1)(parse_state *ps1)))(parse_state *ps);
result* (*chain(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2), result *(*p3)(parse_state *ps3)))(parse_state *ps);
result* (*chainl(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps);
result* (*list(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps);
result* epsilon_p(parse_state *ps);
//result (*semantic(/* fptr to nullary function? */))(parse_state *ps);
result* (*and(result *(*p1)(parse_state *ps1)))(parse_state *ps);
result* (*not(result *(*p1)(parse_state *ps1)))(parse_state *ps);


