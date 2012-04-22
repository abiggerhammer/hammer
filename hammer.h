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

result (*token(const uint8_t *s))(parse_state);
result (*ch(const uint8_t c))(parse_state);
result (*range(const uint8_t lower, const uint8_t upper))(parse_state);
result (*whitespace(/* fptr to parser */))(parse_state);
result (*action(/* fptr to parser, fptr to action */))(parse_state);
result (*join_action(/* fptr to parser */, const uint8_t *sep))(parse_state);
result (*left_factor_action(/* fptr to parser */))(parse_state);
result (*negate(/* fptr to parser */))(parse_state);
result end_p(parse_state);
result nothing_p(parse_state);
result (*sequence(/* array of fptrs! */))(parse_state);
result (*choice(/* array of fptrs */))(parse_state);
result (*butnot(/* fptr to parser1, fptr to parser2 */))(parse_state);
result (*difference(/* fptr to parser1, fptr to parser2 */))(parse_state);
result (*xor(/* fptr to parser1, fptr to parser2 */))(parse_state);
result (*repeat0(/* fptr to parser */))(parse_state);
result (*repeat1(/* fptr to parser */))(parse_state);
result (*repeatN(/* fptr to parser */, const size_t n))(parse_state);
result (*optional(/* fptr to parser */))(parse_state);
void (*expect(/* fptr to parser */))(parse_state);
result (*chain(/* fptr to item parser */, /* fptr to separator parser */, /* fptr to function */))(parse_state);
result (*chainl(/* fptr to parser */, /* fptr to separator parser */))(parse_state);
result (*list(/* fptr to parser */, /* fptr to separator parser */))(parse_state);
result epsilon_p(parse_state);
result (*semantic(/* fptr to nullary function? */))(parse_state);
result (*and(/* fptr to conditional-syntax parser */))(parse_state);
result (*not(/* fptr to conditional-syntax parser */))(parse_state);


