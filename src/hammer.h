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
#define BIT_LITTLE_ENDIAN 0x0
#define BYTE_LITTLE_ENDIAN 0x0

typedef struct input_stream {
  // This should be considered to be a really big value type.
  const uint8_t *input;
  size_t index;
  size_t length;
  char bit_offset;
  char endianness;
} input_stream_t;
  
typedef struct parse_state {
  GHashTable *cache; 
  input_stream_t input_stream;
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

parse_result_t* parse(const parser_t* parser, const uint8_t* input);

const parser_t* token(const uint8_t *s);
const parser_t* ch(const uint8_t c);
const parser_t* range(const uint8_t lower, const uint8_t upper);
const parser_t* whitespace(const parser_t* p);
//const parser_t* action(const parser_t* p, /* fptr to action on AST */);
const parser_t* join_action(const parser_t* p, const uint8_t *sep);
const parser_t* left_faction_action(const parser_t* p);
const parser_t* negate(const parser_t* p);
const parser_t* end_p();
const parser_t* nothing_p();
const parser_t* sequence(const parser_t* p_array[]);
const parser_t* choice(const parser_t* p_array[]);
const parser_t* butnot(const parser_t* p1, const parser_t* p2);
const parser_t* difference(const parser_t* p1, const parser_t* p2);
const parser_t* xor(const parser_t* p1, const parser_t* p2);
const parser_t* repeat0(const parser_t* p);
const parser_t* repeat1(const parser_t* p);
const parser_t* repeat_n(const parser_t* p, const size_t n);
const parser_t* optional(const parser_t* p);
const parser_t* expect(const parser_t* p);
const parser_t* chain(const parser_t* p1, const parser_t* p2, const parser_t* p3);
const parser_t* chainl(const parser_t* p1, const parser_t* p2);
const parser_t* list(const parser_t* p1, const parser_t* p2);
const parser_t* epsilon_p();
//const parser_t* semantic(/* fptr to nullary function? */);
const parser_t* and(const parser_t* p);
const parser_t* not(const parser_t* p);

#endif // #ifndef HAMMER_HAMMER__H
