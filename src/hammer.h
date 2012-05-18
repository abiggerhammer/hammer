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
#include "allocator.h"

#define BYTE_BIG_ENDIAN 0x1
#define BIT_BIG_ENDIAN 0x2
#define BIT_LITTLE_ENDIAN 0x0
#define BYTE_LITTLE_ENDIAN 0x0

typedef int bool;

typedef struct parse_state parse_state_t;

typedef enum token_type {
  TT_NONE,
  TT_BYTES,
  TT_SINT,
  TT_UINT,
  TT_SEQUENCE,
  TT_ERR,
  TT_MAX
} token_type_t;

typedef struct counted_array {
  size_t capacity;
  size_t used;
  arena_t arena;
  void **elements;
} counted_array_t;

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
    counted_array_t *seq; // a sequence of parsed_token_t's
  };
  size_t index;
  char bit_offset;
} parsed_token_t;



/* If a parse fails, the parse result will be NULL.
 * If a parse is successful but there's nothing there (i.e., if end_p succeeds) then there's a parse result but its ast is NULL.
 */
typedef struct parse_result {
  const parsed_token_t *ast;
  arena_t arena;
} parse_result_t;

/* Type of an action to apply to an AST, used in the action() parser. */
typedef parse_result_t* (*action_t)(parse_result_t *p);

/* Type of a boolean attribute-checking function, used in the attr_bool() parser. */
typedef int (*predicate_t)(parse_result_t *p);

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
const parser_t* action(const parser_t* p, const action_t a);

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
 * - if both succeed but p1's result is as long as or shorter than p2's
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
const parser_t* many(const parser_t* p);

/* Given a parser, p, this parser succeeds for one or more repetitions of p. */
const parser_t* many1(const parser_t* p);

/* Given a parser, p, this parser succeeds for exactly N repetitions of p. */
const parser_t* repeat_n(const parser_t* p, const size_t n);

/* Given a parser, p, this parser succeeds with the value p parsed or with an empty result. */
const parser_t* optional(const parser_t* p);

/* Given a parser, p, this parser succeeds if p succeeds, but doesn't include p's result in the result. */
const parser_t* ignore(const parser_t* p);

/* Given a parser, p, and a parser for a separator, sep, this parser matches a (possibly empty) list of things that p can parse, separated by sep.
 * For example, if p is repeat1(range('0','9')) and sep is ch(','), sepBy(p, sep) will match a comma-separated list of integers. 
 */
const parser_t* sepBy(const parser_t* p, const parser_t* sep);

/* Given a parser, p, and a parser for a separator, sep, this parser matches a list of things that p can parse, separated by sep. Unlike sepBy, this ensures that the result has at least one element.
 * For example, if p is repeat1(range('0','9')) and sep is ch(','), sepBy1(p, sep) will match a comma-separated list of integers. 
 */
const parser_t* sepBy1(const parser_t* p, const parser_t* sep);

/* This parser always returns a zero length match, i.e., empty string. */
const parser_t* epsilon_p();

/* This parser attaches an attribute function, which returns true or false, to a parser. The function is evaluated over the parser's result AST. 
 * The parse only succeeds if the attribute function returns true. 
 */
const parser_t* attr_bool(const parser_t* p, const attr_bool_t a);

/* The 'and' parser is a predicate. It asserts that a conditional syntax is satisfied, but consumes no input. 
 * This is useful for lookahead. As an example:
 *
 * Suppose you already have a parser, hex_p, that parses numbers in hexadecimal format (including the leading '0x'). Then
 *   sequence(and(token((const uint8_t*)"0x", 2)), hex_p)
 * checks to see whether there is a leading "0x", *does not* consume the "0x", and then applies hex_p to parse the hex-formatted number.
 *
 * 'and' succeeds if p succeeds, and fails if p fails. Like 'ignore', 'and' does not attach a result to the AST.
 */
const parser_t* and(const parser_t* p);

/* The 'not' parser is a predicate. It asserts that a conditional syntax is *not* satisfied, and consumes no input.
 * As a somewhat contrived example:
 * 
 * Since 'choice' applies its arguments in order, the following parser:
 *   sequence(ch('a'), choice(ch('+'), token((const uint8_t*)"++"), NULL), ch('b'), NULL)
 * will not parse "a++b", because once choice() has succeeded, it will not backtrack and try other alternatives if a later parser in the sequence
 * fails. 
 * Instead, you can force the use of the second alternative by turning the ch('+') alternative into a sequence with not:
 *   sequence(ch('a'), choice(sequence(ch('+'), not(ch('+')), NULL), token((const uint8_t*)"++")), ch('b'), NULL)
 * If the input string is "a+b", the first alternative is applied; if the input string is "a++b", the second alternative is applied.
 */
const parser_t* not(const parser_t* p);

/**
 * Create a parser that just calls out to another, as yet unknown, parser.
 * Note that the inner parser gets bound later, with bind_indirect.
 * This can be used to create recursive parsers.
 */
parser_t *indirect();

/**
 * Set the inner parser of an indirect. See comments on indirect for details.
 */
void bind_indirect(parser_t* indirect, parser_t* inner);

#endif // #ifndef HAMMER_HAMMER__H
