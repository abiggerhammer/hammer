#include "hammer.h"
#include <string.h>

parse_state* from(parse_state *ps, const size_t index) {
  parse_state p = { ps->input, ps->index + index, ps->length - index, ps->cache };
  parse_state *ret = g_new(parse_state, 1);
  *ret = p;
  return ret;
}

const uint8_t* substring(const parse_state *ps, const size_t start, const size_t end) {
  if (end > start && (ps->index + end) < ps->length) {
    gpointer ret = g_malloc(end - start);
    memcpy(ret, ps->input, end - start);
    return (const uint8_t*)ret;
  } else {
    return NULL;
  }
}

const GVariant* at(parse_state *ps, const size_t index) {
  GVariant *ret = NULL;
  if (index + ps->index < ps->length) 
    ret = g_variant_new_byte((ps->input)[index + ps->index]);
  return g_variant_new_maybe(G_VARIANT_TYPE_BYTE, ret);
}

const gchar* to_string(parse_state *ps) {
  return g_strescape(ps->input, NULL);
}

const parse_result* get_cached(parse_state *ps, const size_t pid); /* {
  gpointer p = g_hash_table_lookup(ps->cache, &pid);
  if (NULL != p)
}
							     */
int put_cached(parse_state *ps, const size_t pid, parse_result cached);

parser token(const uint8_t *s) { return NULL; }
parser ch(const uint8_t c) { return NULL; }
parser range(const uint8_t lower, const uint8_t upper) { return NULL; }
parser whitespace(parser p) { return NULL; }
//parser action(parser p, /* fptr to action on AST */) { return NULL; }
parser join_action(parser p, const uint8_t *sep) { return NULL; }
parser left_faction_action(parser p) { return NULL; }
parser negate(parser p) { return NULL; }
parser end_p() { return NULL; }
parser nothing_p() { return NULL; }
parser sequence(parser p_array[]) { return NULL; }
parser choice(parser p_array[]) { return NULL; }
parser butnot(parser p1, parser p2) { return NULL; }
parser difference(parser p1, parser p2) { return NULL; }
parser xor(parser p1, parser p2) { return NULL; }
parser repeat0(parser p) { return NULL; }
parser repeat1(parser p) { return NULL; }
parser repeat_n(parser p, const size_t n) { return NULL; }
parser optional(parser p) { return NULL; }
parser expect(parser p) { return NULL; }
parser chain(parser p1, parser p2, parser p3) { return NULL; }
parser chainl(parser p1, parser p2) { return NULL; }
parser list(parser p1, parser p2) { return NULL; }
parser epsilon_p() { return NULL; }
//parser semantic(/* fptr to nullary function? */) { return NULL; }
parser and(parser p) { return NULL; }
parser not(parser p) { return NULL; }
