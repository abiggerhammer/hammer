/* Internals for Hammer.
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

#ifndef HAMMER_INTERNAL__H
#define HAMMER_INTERNAL__H
#include <glib.h>
#include <err.h>
#include "hammer.h"

#ifdef NDEBUG
#define assert_message(check, message) do { } while(0)
#else
#define assert_message(check, message) do {				\
    if (!(check))							\
      errx(1, "Assertation failed (programmer error): %s", message);	\
  } while(0)
#endif
#define false 0
#define true 1

typedef struct input_stream {
  // This should be considered to be a really big value type.
  const uint8_t *input;
  size_t index;
  size_t length;
  char bit_offset;
  char endianness;
  char overrun;
} input_stream_t;

/* The state of the parser.
 *
 * Members:
 *   cache - a hash table describing the state of the parse, including partial parse_results. It's a hash table from parser_cache_key_t to parser_cache_value_t. 
 *   input_stream - the input stream at this state.
 *   arena - the arena that has been allocated for the parse this state is in.
 *   lr_stack - used in Warth's recursion
 *   recursion_heads - used in Warth's recursion
 *
 */
  
typedef struct parse_state {
  GHashTable *cache; 
  input_stream_t input_stream;
  arena_t arena;
  GQueue *lr_stack;
  GHashTable *recursion_heads;
} parse_state_t;

/* The (location, parser) tuple used to key the cache.
 */

typedef struct parser_cache_key {
  input_stream_t input_pos;
  const parser_t *parser;
} parser_cache_key_t;

/* A value in the cache is either of value Left or Right (this is a 
 * holdover from Scala, which used Either here). Left corresponds to
 * LR_t, which is for left recursion; Right corresponds to 
 * parse_result_t.
 */

typedef enum parser_cache_value_type {
  PC_LEFT,
  PC_RIGHT
} parser_cache_value_type_t;


/* A recursion head.
 *
 * Members:
 *   head_parser -
 *   involved_set -
 *   eval_set - 
 */
typedef struct head {
  const parser_t *head_parser;
  GSList *involved_set;
  GSList *eval_set;
} head_t;


/* A left recursion.
 *
 * Members:
 *   seed -
 *   rule -
 *   head -
 */
typedef struct LR {
  parse_result_t *seed;
  const parser_t *rule;
  head_t *head;
} LR_t;

/* Tagged union for values in the cache: either LR's (Left) or 
 * parse_result_t's (Right).
 */
typedef struct parser_cache_value {
  parser_cache_value_type_t value_type;
  union {
    LR_t *left;
    parse_result_t *right;
  };
} parser_cache_value_t;

typedef unsigned int *charset;

static inline charset new_charset() {
  charset cs = g_new0(unsigned int, 256 / sizeof(unsigned int));
  return cs;
}

static inline int charset_isset(charset cs, uint8_t pos) {
  return !!(cs[pos / sizeof(*cs)] & (1 << (pos % sizeof(*cs))));
}

static inline void charset_set(charset cs, uint8_t pos, int val) {
  cs[pos / sizeof(*cs)] =
    val
    ? cs[pos / sizeof(*cs)] |  (1 << (pos % sizeof(*cs)))
    : cs[pos / sizeof(*cs)] & ~(1 << (pos % sizeof(*cs)));
}

// TODO(thequux): Set symbol visibility for these functions so that they aren't exported.

long long read_bits(input_stream_t* state, int count, char signed_p);
parse_result_t* do_parse(const parser_t* parser, parse_state_t *state);
void put_cached(parse_state_t *ps, const parser_t *p, parse_result_t *cached);
guint djbhash(const uint8_t *buf, size_t len);
char* write_result_unamb(const parsed_token_t* tok);
void pprint(const parsed_token_t* tok, int indent, int delta);

#endif // #ifndef HAMMER_INTERNAL__H
