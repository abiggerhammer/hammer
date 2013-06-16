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
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "allocator.h"

#define BYTE_BIG_ENDIAN 0x1
#define BIT_BIG_ENDIAN 0x2
#define BIT_LITTLE_ENDIAN 0x0
#define BYTE_LITTLE_ENDIAN 0x0

typedef int bool;

typedef struct HParseState_ HParseState;

typedef enum HParserBackend_ {
  PB_MIN = 0,
  PB_PACKRAT = PB_MIN, // PB_MIN is always the default.
  PB_REGULAR,
  PB_LLk,
  PB_LALR,
  PB_GLR,	// Not Implemented
  PB_MAX = PB_LALR
} HParserBackend;

typedef enum HTokenType_ {
  // Before you change the explicit values of these, think of the poor bindings ;_;
  TT_NONE = 1,
  TT_BYTES = 2,
  TT_SINT = 4,
  TT_UINT = 8,
  TT_SEQUENCE = 16,
  TT_RESERVED_1, // reserved for backend-specific internal use
  TT_ERR = 32,
  TT_USER = 64,
  TT_MAX
} HTokenType;

typedef struct HCountedArray_ {
  size_t capacity;
  size_t used;
  HArena * arena;
  struct HParsedToken_ **elements;
} HCountedArray;

typedef struct HBytes_ {
  const uint8_t *token;
  size_t len;
} HBytes;

typedef struct HParsedToken_ {
  HTokenType token_type;
  union {
    HBytes bytes;
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    HCountedArray *seq; // a sequence of HParsedToken's
    void *user;
  };
  size_t index;
  char bit_offset;
} HParsedToken;

/**
 * The result of a successful parse. Note that this may reference the
 * input string.
 *
 * If a parse fails, the parse result will be NULL.
 * If a parse is successful but there's nothing there (i.e., if end_p 
 * succeeds) then there's a parse result but its ast is NULL.
 */
typedef struct HParseResult_ {
  const HParsedToken *ast;
  long long bit_length;
  HArena * arena;
} HParseResult;

/**
 * TODO: document me.
 * Relevant functions: h_bit_writer_new, h_bit_writer_put, h_bit_writer_get_buffer, h_bit_writer_free
 */
typedef struct HBitWriter_ HBitWriter;

/**
 * Type of an action to apply to an AST, used in the action() parser. 
 * It can be any (user-defined) function that takes a HParseResult*
 * and returns a HParsedToken*. (This is so that the user doesn't 
 * have to worry about memory allocation; action() does that for you.)
 * Note that the tagged union in HParsedToken* supports user-defined 
 * types, so you can create your own token types (corresponding to, 
 * say, structs) and stuff values for them into the void* in the 
 * tagged union in HParsedToken. 
 */
typedef HParsedToken* (*HAction)(const HParseResult *p);

/**
 * Type of a boolean attribute-checking function, used in the 
 * attr_bool() parser. It can be any (user-defined) function that takes
 * a HParseResult* and returns true or false. 
 */
typedef bool (*HPredicate)(HParseResult *p);

typedef struct HCFChoice_ HCFChoice;
typedef struct HRVMProg_ HRVMProg;
typedef struct HParserVtable_ HParserVtable;

typedef struct HParser_ {
  const HParserVtable *vtable;
  HParserBackend backend;
  void* backend_data;
  void *env;
  HCFChoice *desugared; /* if the parser can be desugared, its desugared form */
} HParser;

// {{{ Stuff for benchmarking
typedef struct HParserTestcase_ {
  unsigned char* input;
  size_t length;
  char* output_unambiguous;
} HParserTestcase;

typedef struct HCaseResult_ {
  bool success;
  union {
    const char* actual_results; // on failure, filled in with the results of h_write_result_unamb
    size_t parse_time; // on success, filled in with time for a single parse, in nsec
  };
} HCaseResult;

typedef struct HBackendResults_ {
  HParserBackend backend;
  bool compile_success;
  size_t n_testcases;
  size_t failed_testcases; // actually a count...
  HCaseResult *cases;
} HBackendResults;

typedef struct HBenchmarkResults_ {
  size_t len;
  HBackendResults *results;
} HBenchmarkResults;
// }}}

// {{{ Preprocessor definitions
#define HAMMER_FN_DECL_NOARG(rtype_t, name)		\
  rtype_t name(void);					\
  rtype_t name##__m(HAllocator* mm__)

#define HAMMER_FN_DECL(rtype_t, name, ...)		\
  rtype_t name(__VA_ARGS__);				\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__)

#define HAMMER_FN_DECL_ATTR(attr, rtype_t, name, ...)			\
  rtype_t name(__VA_ARGS__) attr;					\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__) attr

#define HAMMER_FN_DECL_VARARGS(rtype_t, name, ...)			\
  rtype_t name(__VA_ARGS__, ...);					\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__, ...);		\
  rtype_t name##__mv(HAllocator* mm__, __VA_ARGS__, va_list ap);	\
  rtype_t name##__v(__VA_ARGS__, va_list ap);   \
  rtype_t name##__a(void *args[]);  \
  rtype_t name##__ma(HAllocator *mm__, void *args[])

// Note: this drops the attributes on the floor for the __v versions
#define HAMMER_FN_DECL_VARARGS_ATTR(attr, rtype_t, name, ...)		\
  rtype_t name(__VA_ARGS__, ...) attr;					\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__, ...) attr;		\
  rtype_t name##__mv(HAllocator* mm__, __VA_ARGS__, va_list ap);	\
  rtype_t name##__v(__VA_ARGS__, va_list ap); \
  rtype_t name##__a(void *args[]);  \
  rtype_t name##__ma(HAllocator *mm__, void *args[])

// }}}


/**
 * Top-level function to call a parser that has been built over some
 * piece of input (of known size).
 */
HAMMER_FN_DECL(HParseResult*, h_parse, const HParser* parser, const uint8_t* input, size_t length);

/**
 * Given a string, returns a parser that parses that string value. 
 * 
 * Result token type: TT_BYTES
 */
HAMMER_FN_DECL(HParser*, h_token, const uint8_t *str, const size_t len);

/**
 * Given a single character, returns a parser that parses that 
 * character. 
 * 
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_ch, const uint8_t c);

/**
 * Given two single-character bounds, lower and upper, returns a parser
 * that parses a single character within the range [lower, upper] 
 * (inclusive). 
 * 
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_ch_range, const uint8_t lower, const uint8_t upper);

/**
 * Given an integer parser, p, and two integer bounds, lower and upper,
 * returns a parser that parses an integral value within the range 
 * [lower, upper] (inclusive).
 */
HAMMER_FN_DECL(HParser*, h_int_range, const HParser *p, const int64_t lower, const int64_t upper);

/**
 * Returns a parser that parses the specified number of bits. sign == 
 * true if signed, false if unsigned. 
 *
 * Result token type: TT_SINT if sign == true, TT_UINT if sign == false
 */
HAMMER_FN_DECL(HParser*, h_bits, size_t len, bool sign);

/**
 * Returns a parser that parses a signed 8-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int64);

/**
 * Returns a parser that parses a signed 4-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int32);

/**
 * Returns a parser that parses a signed 2-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int16);

/**
 * Returns a parser that parses a signed 1-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int8);

/**
 * Returns a parser that parses an unsigned 8-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint64);

/**
 * Returns a parser that parses an unsigned 4-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint32);

/**
 * Returns a parser that parses an unsigned 2-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint16);

/**
 * Returns a parser that parses an unsigned 1-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint8);

/**
 * Given another parser, p, returns a parser that skips any whitespace 
 * and then applies p. 
 *
 * Result token type: p's result type
 */
HAMMER_FN_DECL(HParser*, h_whitespace, const HParser* p);

/**
 * Given two parsers, p and q, returns a parser that parses them in
 * sequence but only returns p's result.
 *
 * Result token type: p's result type
 */
HAMMER_FN_DECL(HParser*, h_left, const HParser* p, const HParser* q);

/**
 * Given two parsers, p and q, returns a parser that parses them in
 * sequence but only returns q's result.
 *
 * Result token type: q's result type
 */
HAMMER_FN_DECL(HParser*, h_right, const HParser* p, const HParser* q);

/**
 * Given three parsers, p, x, and q, returns a parser that parses them in
 * sequence but only returns x's result.
 *
 * Result token type: x's result type
 */
HAMMER_FN_DECL(HParser*, h_middle, const HParser* p, const HParser* x, const HParser* q);

/**
 * Given another parser, p, and a function f, returns a parser that 
 * applies p, then applies f to everything in the AST of p's result. 
 *
 * Result token type: any
 */
HAMMER_FN_DECL(HParser*, h_action, const HParser* p, const HAction a);

/**
 * Parse a single character in the given charset. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_in, const uint8_t *charset, size_t length);

/**
 * Parse a single character *NOT* in the given charset. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_not_in, const uint8_t *charset, size_t length);

/**
 * A no-argument parser that succeeds if there is no more input to 
 * parse. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL_NOARG(HParser*, h_end_p);

/**
 * This parser always fails. 
 *
 * Result token type: NULL. Always.
 */
HAMMER_FN_DECL_NOARG(HParser*, h_nothing_p);

/**
 * Given a null-terminated list of parsers, apply each parser in order.
 * The parse succeeds only if all parsers succeed. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL_VARARGS_ATTR(__attribute__((sentinel)), HParser*, h_sequence, HParser* p);

/**
 * Given an array of parsers, p_array, apply each parser in order. The 
 * first parser to succeed is the result; if no parsers succeed, the 
 * parse fails. 
 *
 * Result token type: The type of the first successful parser's result.
 */
HAMMER_FN_DECL_VARARGS_ATTR(__attribute__((sentinel)), HParser*, h_choice, HParser* p);

/**
 * Given two parsers, p1 and p2, this parser succeeds in the following 
 * cases: 
 * - if p1 succeeds and p2 fails
 * - if both succeed but p1's result is as long as or longer than p2's
 *
 * Result token type: p1's result type.
 */
HAMMER_FN_DECL(HParser*, h_butnot, const HParser* p1, const HParser* p2);

/**
 * Given two parsers, p1 and p2, this parser succeeds in the following 
 * cases:
 * - if p1 succeeds and p2 fails
 * - if both succeed but p2's result is shorter than p1's
 *
 * Result token type: p1's result type.
 */
HAMMER_FN_DECL(HParser*, h_difference, const HParser* p1, const HParser* p2);

/**
 * Given two parsers, p1 and p2, this parser succeeds if *either* p1 or
 * p2 succeed, but not if they both do.
 *
 * Result token type: The type of the result of whichever parser succeeded.
 */
HAMMER_FN_DECL(HParser*, h_xor, const HParser* p1, const HParser* p2);

/**
 * Given a parser, p, this parser succeeds for zero or more repetitions
 * of p. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_many, const HParser* p);

/**
 * Given a parser, p, this parser succeeds for one or more repetitions 
 * of p. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_many1, const HParser* p);

/**
 * Given a parser, p, this parser succeeds for exactly N repetitions 
 * of p. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_repeat_n, const HParser* p, const size_t n);

/**
 * Given a parser, p, this parser succeeds with the value p parsed or 
 * with an empty result. 
 *
 * Result token type: If p succeeded, the type of its result; if not, TT_NONE.
 */
HAMMER_FN_DECL(HParser*, h_optional, const HParser* p);

/**
 * Given a parser, p, this parser succeeds if p succeeds, but doesn't 
 * include p's result in the result. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL(HParser*, h_ignore, const HParser* p);

/**
 * Given a parser, p, and a parser for a separator, sep, this parser 
 * matches a (possibly empty) list of things that p can parse, 
 * separated by sep.
 * For example, if p is repeat1(range('0','9')) and sep is ch(','), 
 * sepBy(p, sep) will match a comma-separated list of integers. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_sepBy, const HParser* p, const HParser* sep);

/**
 * Given a parser, p, and a parser for a separator, sep, this parser matches a list of things that p can parse, separated by sep. Unlike sepBy, this ensures that the result has at least one element.
 * For example, if p is repeat1(range('0','9')) and sep is ch(','), sepBy1(p, sep) will match a comma-separated list of integers. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_sepBy1, const HParser* p, const HParser* sep);

/**
 * This parser always returns a zero length match, i.e., empty string. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL_NOARG(HParser*, h_epsilon_p);

/**
 * This parser applies its first argument to read an unsigned integer
 * value, then applies its second argument that many times. length 
 * should parse an unsigned integer value; this is checked at runtime.
 * Specifically, the token_type of the returned token must be TT_UINT.
 * In future we might relax this to include TT_USER but don't count on it.
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_length_value, const HParser* length, const HParser* value);

/**
 * This parser attaches a predicate function, which returns true or 
 * false, to a parser. The function is evaluated over the parser's 
 * result. 
 *
 * The parse only succeeds if the attribute function returns true. 
 *
 * attr_bool will check whether p's result exists and whether p's 
 * result AST exists; you do not need to check for this in your 
 * predicate function.
 * 
 * Result token type: p's result type if pred succeeded, NULL otherwise.
 */
HAMMER_FN_DECL(HParser*, h_attr_bool, const HParser* p, HPredicate pred);

/**
 * The 'and' parser asserts that a conditional syntax is satisfied, 
 * but doesn't consume that conditional syntax. 
 * This is useful for lookahead. As an example:
 *
 * Suppose you already have a parser, hex_p, that parses numbers in 
 * hexadecimal format (including the leading '0x'). Then
 *   sequence(and(token((const uint8_t*)"0x", 2)), hex_p)
 * checks to see whether there is a leading "0x", *does not* consume 
 * the "0x", and then applies hex_p to parse the hex-formatted number.
 *
 * 'and' succeeds if p succeeds, and fails if p fails. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL(HParser*, h_and, const HParser* p);

/**
 * The 'not' parser asserts that a conditional syntax is *not* 
 * satisfied, but doesn't consume that conditional syntax.
 * As a somewhat contrived example:
 * 
 * Since 'choice' applies its arguments in order, the following parser:
 *   sequence(ch('a'), choice(ch('+'), token((const uint8_t*)"++"), NULL), ch('b'), NULL)
 * will not parse "a++b", because once choice() has succeeded, it will 
 * not backtrack and try other alternatives if a later parser in the 
 * sequence fails. 
 * Instead, you can force the use of the second alternative by turning 
 * the ch('+') alternative into a sequence with not:
 *   sequence(ch('a'), choice(sequence(ch('+'), not(ch('+')), NULL), token((const uint8_t*)"++")), ch('b'), NULL)
 * If the input string is "a+b", the first alternative is applied; if 
 * the input string is "a++b", the second alternative is applied.
 * 
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL(HParser*, h_not, const HParser* p);

/**
 * Create a parser that just calls out to another, as yet unknown, 
 * parser.
 * Note that the inner parser gets bound later, with bind_indirect.
 * This can be used to create recursive parsers.
 *
 * Result token type: the type of whatever parser is bound to it with
 * bind_indirect().
 */
HAMMER_FN_DECL_NOARG(HParser*, h_indirect);

/**
 * Set the inner parser of an indirect. See comments on indirect for 
 * details.
 */
HAMMER_FN_DECL(void, h_bind_indirect, HParser* indirect, const HParser* inner);

/**
 * Free the memory allocated to an HParseResult when it is no longer needed.
 */
HAMMER_FN_DECL(void, h_parse_result_free, HParseResult *result);

// Some debugging aids
/**
 * Format token into a compact unambiguous form. Useful for parser test cases.
 * Caller is responsible for freeing the result.
 */
HAMMER_FN_DECL(char*, h_write_result_unamb, const HParsedToken* tok);
/**
 * Format token to the given output stream. Indent starting at
 * [indent] spaces, with [delta] spaces between levels.
 */
HAMMER_FN_DECL(void, h_pprint, FILE* stream, const HParsedToken* tok, int indent, int delta);

/**
 * Build parse tables for the given parser backend. See the
 * documentation for the parser backend in question for information
 * about the [params] parameter, or just pass in NULL for the defaults.
 *
 * Returns -1 if grammar cannot be compiled with the specified options; 0 otherwise.
 */
HAMMER_FN_DECL(int, h_compile, HParser* parser, HParserBackend backend, const void* params);

/**
 * TODO: Document me
 */
HBitWriter *h_bit_writer_new(HAllocator* mm__);

/**
 * TODO: Document me
 */
void h_bit_writer_put(HBitWriter* w, unsigned long long data, size_t nbits);

/**
 * TODO: Document me
 * Must not free [w] until you're done with the result.
 * [len] is in bytes.
 */
const uint8_t* h_bit_writer_get_buffer(HBitWriter* w, size_t *len);

/**
 * TODO: Document me
 */
void h_bit_writer_free(HBitWriter* w);

// General-purpose actions for use with h_action
// XXX to be consolidated with glue.h when merged upstream
HParsedToken *h_act_first(const HParseResult *p);
HParsedToken *h_act_second(const HParseResult *p);
HParsedToken *h_act_last(const HParseResult *p);
HParsedToken *h_act_flatten(const HParseResult *p);
HParsedToken *h_act_ignore(const HParseResult *p);

// {{{ Benchmark functions
HAMMER_FN_DECL(HBenchmarkResults *, h_benchmark, HParser* parser, HParserTestcase* testcases);
void h_benchmark_report(FILE* stream, HBenchmarkResults* results);
void h_benchmark_dump_optimized_code(FILE* stream, HBenchmarkResults* results);
// }}}

#endif // #ifndef HAMMER_HAMMER__H
