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
 *   cache - a hash table describing the state of the parse, including partial parse_results. It's a hash table from parser_cache_key_t to parse_state_t. 
 *
 */
#define BYTE_BIG_ENDIAN 0x1
#define BIT_BIG_ENDIAN 0x2
#define BIT_LITTLE_ENDIAN 0x0
#define BYTE_LITTLE_ENDIAN 0x0

typedef int bool;
typedef struct input_stream {
  // This should be considered to be a really big value type.
  const uint8_t *input;
  size_t index;
  size_t length;
  char bit_offset;
  char endianness;
  char overrun;
} input_stream_t;
  
typedef struct parse_state {
  GHashTable *cache; 
  input_stream_t input_stream;
} parse_state_t;

typedef enum token_type {
  TT_NONE,
  TT_BYTES,
  TT_SINT,
  TT_UINT,
  TT_SEQUENCE,
  TT_MAX
} token_type_t;

typedef struct parsed_token {
  token_type_t token_type;
  union {
    struct {
      const uint8_t *token;
      size_t len;
    } bytes;
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    GSequence *seq;
  };
} parsed_token_t;

/* If a parse fails, the parse result will be NULL.
 * If a parse is successful but there's nothing there (i.e., if end_p succeeds) then there's a parse result but its ast is NULL.
 */
typedef struct parse_result {
  const parsed_token_t *ast;
} parse_result_t;

typedef struct parser {
  parse_result_t* (*fn)(void *env, parse_state_t *state);
  void *env;
} parser_t;

parse_result_t* parse(const parser_t* parser, const uint8_t* input, size_t length);

/* Given a string, returns a parser that parses that string value. */
const parser_t* token(const uint8_t *str, const size_t len);

/* Given a single character, returns a parser that parses that character. */
const parser_t* ch(const uint8_t c);

/* Given two single-character bounds, lower and upper, returns a parser that parses a single character within the range [lower, upper] (inclusive). */
const parser_t* range(const uint8_t lower, const uint8_t upper);

/* Returns a parser that parses the specified number of bits. sign == true if signed, false if unsigned. */
const parser_t* bits(size_t len, bool sign);

/* Returns a parser that parses a signed 8-byte integer value. */
const parser_t* int64();

/* Returns a parser that parses a signed 4-byte integer value. */
const parser_t* int32();

/* Returns a parser that parses a signed 2-byte integer value. */
const parser_t* int16();

/* Returns a parser that parses a signed 1-byte integer value. */
const parser_t* int8();

/* Returns a parser that parses an unsigned 8-byte integer value. */
const parser_t* uint64();

/* Returns a parser that parses an unsigned 4-byte integer value. */
const parser_t* uint32();

/* Returns a parser that parses an unsigned 2-byte integer value. */
const parser_t* uint16();

/* Returns a parser that parses an unsigned 1-byte integer value. */
const parser_t* uint8();

/* Returns a parser that parses a double-precision floating-point value. */
const parser_t* float64();

/* Returns a parser that parses a single-precision floating-point value. */
const parser_t* float32();

/* Given another parser, p, returns a parser that skips any whitespace and then applies p. */
const parser_t* whitespace(const parser_t* p);

/* Given another parser, p, and a function f, returns a parser that applies p, then applies f to everything in the AST of p's result. */
//const parser_t* action(const parser_t* p, /* fptr to action on AST */);

const parser_t* left_factor_action(const parser_t* p);

/* Parse a single character *NOT* in charset */
const parser_t* not_in(const uint8_t *charset, int length);

/* A no-argument parser that succeeds if there is no more input to parse. */
const parser_t* end_p();

/* This parser always fails. */
const parser_t* nothing_p();

/* Given an null-terminated list of parsers, apply each parser in order. The parse succeeds only if all parsers succeed. */
const parser_t* sequence(const parser_t* p, ...) __attribute__((sentinel));

/* Given an array of parsers, p_array, apply each parser in order. The first parser to succeed is the result; if no parsers succeed, the parse fails. */
const parser_t* choice(const parser_t* p, ...) __attribute__((sentinel));

/* Given two parsers, p1 and p2, this parser succeeds in the following cases: 
 * - if p1 succeeds and p2 fails
 * - if both succeed but p1's result is shorter than p2's
 */
const parser_t* butnot(const parser_t* p1, const parser_t* p2);

/* Given two parsers, p1 and p2, this parser succeeds in the following cases:
 * - if p1 succeeds and p2 fails
 * - if both succeed but p2's result is shorter than p1's
 */
const parser_t* difference(const parser_t* p1, const parser_t* p2);

/* Given two parsers, p1 and p2, this parser succeeds if *either* p1 or p2 succeed, but not if they both do.
 */
const parser_t* xor(const parser_t* p1, const parser_t* p2);

/* Given a parser, p, this parser succeeds for zero or more repetitions of p. */
const parser_t* repeat0(const parser_t* p);

/* Given a parser, p, this parser succeeds for one or more repetitions of p. */
const parser_t* repeat1(const parser_t* p);

/* Given a parser, p, this parser succeeds for exactly N repetitions of p. */
const parser_t* repeat_n(const parser_t* p, const size_t n);

/* Given a parser, p, this parser succeeds with the value p parsed or with an empty result. */
const parser_t* optional(const parser_t* p);

/* Given a parser, p, this parser succeeds if p succeeds, but doesn't include p's result in the result. */
const parser_t* ignore(const parser_t* p);

const parser_t* chain(const parser_t* p1, const parser_t* p2, const parser_t* p3);
const parser_t* chainl(const parser_t* p1, const parser_t* p2);
const parser_t* list(const parser_t* p1, const parser_t* p2);
const parser_t* epsilon_p();
//const parser_t* semantic(/* fptr to nullary function? */);
const parser_t* and(const parser_t* p);
const parser_t* not(const parser_t* p);

#endif // #ifndef HAMMER_HAMMER__H
