/* Parser combinators for binary formats.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef HAMMER_HAMMER__H
#define HAMMER_HAMMER__H
#include <glib.h>
#include <stdint.h>

/* The state of the parser.
 *
 * Members:
 *   input - the entire string being parsed
 *   index - current position in input
 *   length - size of input
 * THE FOLLOWING DESCRIBES HOW JSPARSE DOES IT. OUR MILEAGE MAY VARY.
 *   cache - a hash table describing the state of the parse, including partial parse_results. 
 *           It's actually a hash table of [parser_id, hash_table[index, parse_result]],
 *           where the parser id is incremented as the parse goes along (parsers that have
 *           already been applied once don't get a new parser_id ... but the global variable
 *           still increments? not sure why that is, need to debug some), and the locations
 *           at which it's been applied are memoized.
 *
 */
#define BYTE_BIG_ENDIAN 0x1
#define BIT_BIG_ENDIAN 0x2

typedef struct parse_state {
  const uint8_t *input;
  GHashTable *cache; 
  size_t index;
  size_t length;
  char bit_offset;
  char endianness;
} parse_state_t;

typedef struct parse_result {
  const uint8_t *remaining;
  const uint8_t *matched;
  const GSequence *ast;
} parse_result_t;

typedef struct parser {
  parse_result_t* (*fn)(void* env, parse_state_t *state);
  void* env;
} parser_t;

parse_result_t* parse(parser_t* parser, const uint8_t* input);

parser_t* token(const uint8_t *s);
parser_t* ch(const uint8_t c);
parser_t* range(const uint8_t lower, const uint8_t upper);
parser_t* whitespace(parser_t* p);
//parser_t* action(parser_t* p, /* fptr to action on AST */);
parser_t* join_action(parser_t* p, const uint8_t *sep);
parser_t* left_faction_action(parser_t* p);
parser_t* negate(parser_t* p);
parser_t* end_p();
parser_t* nothing_p();
parser_t* sequence(parser_t* p_array[]);
parser_t* choice(parser_t* p_array[]);
parser_t* butnot(parser_t* p1, parser_t* p2);
parser_t* difference(parser_t* p1, parser_t* p2);
parser_t* xor(parser_t* p1, parser_t* p2);
parser_t* repeat0(parser_t* p);
parser_t* repeat1(parser_t* p);
parser_t* repeat_n(parser_t* p, const size_t n);
parser_t* optional(parser_t* p);
parser_t* expect(parser_t* p);
parser_t* chain(parser_t* p1, parser_t* p2, parser_t* p3);
parser_t* chainl(parser_t* p1, parser_t* p2);
parser_t* list(parser_t* p1, parser_t* p2);
parser_t* epsilon_p();
//parser_t* semantic(/* fptr to nullary function? */);
parser_t* and(parser_t* p);
parser_t* not(parser_t* p);

#endif // #ifndef HAMMER_HAMMER__H
