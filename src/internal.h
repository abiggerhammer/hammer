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
#include "hammer.h"

#define false 0
#define true 1

typedef struct parser_cache_key {
  input_stream_t input_pos;
  const parser_t *parser;
} parser_cache_key_t;

typedef enum parser_cache_value_type {
  PC_LEFT,
  PC_RIGHT
} parser_cache_value_type_t;

typedef struct head {
  parser_t *head_parser;
  GSList *involved_set;
  GSList *eval_set;
} head_t;

typedef struct LR {
  parse_result_t *seed;
  const parser_t *rule;
  head_t *head;
} LR_t;

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
