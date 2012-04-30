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

#include "hammer.h"
#include <string.h>
/* TODO(thequux): rewrite to follow new parse_state_t layout
parse_state_t* from(parse_state_t *ps, const size_t index) {
  parse_state_t p = { ps->input, ps->index + index, ps->length - index, ps->cache };
  parse_state_t *ret = g_new(parse_state_t, 1);
  *ret = p;
  return ret;
}
*/
const uint8_t* substring(const parse_state_t *ps, const size_t start, const size_t end) {
  if (end > start && (ps->input_stream.index + end) < ps->input_stream.length) {
    gpointer ret = g_malloc(end - start);
    memcpy(ret, ps->input_stream.input, end - start);
    return (const uint8_t*)ret;
  } else {
    return NULL;
  }
}

const GVariant* at(parse_state_t *ps, const size_t index) {
  GVariant *ret = NULL;
  if (index + ps->input_stream.index < ps->input_stream.length) 
    ret = g_variant_new_byte((ps->input_stream.input)[index + ps->input_stream.index]);
  return g_variant_new_maybe(G_VARIANT_TYPE_BYTE, ret);
}

const gchar* to_string(parse_state_t *ps) {
  return g_strescape((const gchar*)(ps->input_stream.input), NULL);
}

uint8_t djbhash(size_t index, char bit_offset) {
  unsigned int hash = 5381;
  for (uint8_t i = 0; i < sizeof(size_t); ++i) {
    hash = hash * 33 + (index & 0xFF);
    index >>= 8;
  }
  hash = hash * 33 + bit_offset;
  return hash;
}

parse_result_t* get_cached(parse_state_t *ps, const parser_t *p) {
  gpointer t = g_hash_table_lookup(ps->cache, p);
  if (NULL != t) {
    parse_result_t* ret = g_hash_table_lookup(t, GUINT_TO_POINTER(djbhash(ps->input_stream.index, 
									  ps->input_stream.length)));
    if (NULL != ret) {
      return ret;
    } else {
      // TODO(mlp): need a return value for "this parser was in the cache but nothing was at this location"
      return NULL;
    }
  } else {
    // TODO(mlp): need a return value for "this parser wasn't in the cache"
    return NULL;
  }
}

int put_cached(parse_state_t *ps, const parser_t *p, parse_result_t *cached) {
  gpointer t = g_hash_table_lookup(ps->cache, p);
  if (NULL != t) {
    g_hash_table_insert(t, GUINT_TO_POINTER(djbhash(ps->input_stream.index, ps->input_stream.length)), (gpointer)cached); 
  } else {
    GHashTable *t = g_hash_table_new(g_direct_hash, g_direct_equal);
    g_hash_table_insert(t, GUINT_TO_POINTER(djbhash(ps->input_stream.index, ps->input_stream.length)), (gpointer)cached);
    g_hash_table_insert(ps->cache, (parser_t*)p, t);
  }
}

const parser_t* token(const uint8_t *s) { return NULL; }
const parser_t* ch(const uint8_t c) { return NULL; }
const parser_t* range(const uint8_t lower, const uint8_t upper) { return NULL; }
const parser_t* whitespace(const parser_t* p) { return NULL; }
//const parser_t* action(const parser_t* p, /* fptr to action on AST */) { return NULL; }
const parser_t* join_action(const parser_t* p, const uint8_t *sep) { return NULL; }
const parser_t* left_faction_action(const parser_t* p) { return NULL; }
const parser_t* negate(const parser_t* p) { return NULL; }
const parser_t* end_p() { return NULL; }
const parser_t* nothing_p() { return NULL; }
const parser_t* sequence(const parser_t* p_array[]) { return NULL; }
const parser_t* choice(const parser_t* p_array[]) { return NULL; }
const parser_t* butnot(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* difference(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* xor(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* repeat0(const parser_t* p) { return NULL; }
const parser_t* repeat1(const parser_t* p) { return NULL; }
const parser_t* repeat_n(const parser_t* p, const size_t n) { return NULL; }
const parser_t* optional(const parser_t* p) { return NULL; }
const parser_t* expect(const parser_t* p) { return NULL; }
const parser_t* chain(const parser_t* p1, const parser_t* p2, const parser_t* p3) { return NULL; }
const parser_t* chainl(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* list(const parser_t* p1, const parser_t* p2) { return NULL; }
const parser_t* epsilon_p() { return NULL; }
//const parser_t* semantic(/* fptr to nullary function? */) { return NULL; }
const parser_t* and(const parser_t* p) { return NULL; }
const parser_t* not(const parser_t* p) { return NULL; }

parse_result_t* parse(const parser_t* parser, const uint8_t* input) { return NULL; }
