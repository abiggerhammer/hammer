#include "hammer.h"
#include <string.h>

parse_state_t* from(parse_state_t *ps, const size_t index) {
  parse_state_t p = { ps->input, ps->index + index, ps->length - index, ps->cache };
  parse_state_t *ret = g_new(parse_state_t, 1);
  *ret = p;
  return ret;
}

const uint8_t* substring(const parse_state_t *ps, const size_t start, const size_t end) {
  if (end > start && (ps->index + end) < ps->length) {
    gpointer ret = g_malloc(end - start);
    memcpy(ret, ps->input, end - start);
    return (const uint8_t*)ret;
  } else {
    return NULL;
  }
}

const GVariant* at(parse_state_t *ps, const size_t index) {
  GVariant *ret = NULL;
  if (index + ps->index < ps->length) 
    ret = g_variant_new_byte((ps->input)[index + ps->index]);
  return g_variant_new_maybe(G_VARIANT_TYPE_BYTE, ret);
}

const gchar* to_string(parse_state_t *ps) {
  return g_strescape(ps->input, NULL);
}

const parse_result_t* get_cached(parse_state_t *ps, const size_t pid); /* {
  gpointer p = g_hash_table_lookup(ps->cache, &pid);
  if (NULL != p)
}
							     */
int put_cached(parse_state_t *ps, const size_t pid, parse_result_t *cached);

parser_t* token(const uint8_t *s) { return NULL; }
parser_t* ch(const uint8_t c) { return NULL; }
parser_t* range(const uint8_t lower, const uint8_t upper) { return NULL; }
parser_t* whitespace(parser_t* p) { return NULL; }
//parser_t* action(parser_t* p, /* fptr to action on AST */) { return NULL; }
parser_t* join_action(parser_t* p, const uint8_t *sep) { return NULL; }
parser_t* left_faction_action(parser_t* p) { return NULL; }
parser_t* negate(parser_t* p) { return NULL; }
parser_t* end_p() { return NULL; }
parser_t* nothing_p() { return NULL; }
parser_t* sequence(parser_t* p_array[]) { return NULL; }
parser_t* choice(parser_t* p_array[]) { return NULL; }
parser_t* butnot(parser_t* p1, parser_t* p2) { return NULL; }
parser_t* difference(parser_t* p1, parser_t* p2) { return NULL; }
parser_t* xor(parser_t* p1, parser_t* p2) { return NULL; }
parser_t* repeat0(parser_t* p) { return NULL; }
parser_t* repeat1(parser_t* p) { return NULL; }
parser_t* repeat_n(parser_t* p, const size_t n) { return NULL; }
parser_t* optional(parser_t* p) { return NULL; }
parser_t* expect(parser_t* p) { return NULL; }
parser_t* chain(parser_t* p1, parser_t* p2, parser_t* p3) { return NULL; }
parser_t* chainl(parser_t* p1, parser_t* p2) { return NULL; }
parser_t* list(parser_t* p1, parser_t* p2) { return NULL; }
parser_t* epsilon_p() { return NULL; }
//parser_t* semantic(/* fptr to nullary function? */) { return NULL; }
parser_t* and(parser_t* p) { return NULL; }
parser_t* not(parser_t* p) { return NULL; }
