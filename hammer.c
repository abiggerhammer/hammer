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

const result* get_cached(parse_state *ps, const size_t pid); /* {
  gpointer p = g_hash_table_lookup(ps->cache, &pid);
  if (NULL != p)
}
							     */
int put_cached(parse_state *ps, const size_t pid, result cached);

result* (*token(const uint8_t *s))(parse_state *ps) { return NULL; }
result* (*ch(const uint8_t c))(parse_state *ps) { return NULL; }
result* (*range(const uint8_t lower, const uint8_t upper))(parse_state *ps) { return NULL; }
result* (*whitespace(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
//result (*action(result *(*p1)(parse_state *ps1), /* fptr to action on AST */))(parse_state *ps) { return NULL; }
result* (*join_action(result *(*p1)(parse_state *ps1), const uint8_t *sep))(parse_state *ps) { return NULL; }
result* (*left_factor_action(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
result* (*negate(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
result* end_p(parse_state *ps) { return NULL; }
result* nothing_p(parse_state *ps) { return NULL; }
result* (*sequence(result *(*(*p1)(parse_state *ps1))))(parse_state *ps) { return NULL; }
result* (*choice(result *(*(*p1)(parse_state *ps1))))(parse_state *ps) { return NULL; }
result* (*butnot(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps) { return NULL; }
result* (*difference(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps) { return NULL; }
result* (*xor(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps) { return NULL; }
result* (*repeat0(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
result* (*repeat1(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
result* (*repeatN(result *(*p1)(parse_state *ps1), const size_t n))(parse_state *ps) { return NULL; }
result* (*optional(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
void (*expect(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
result* (*chain(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2), result *(*p3)(parse_state *ps3)))(parse_state *ps) { return NULL; }
result* (*chainl(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps) { return NULL; }
result* (*list(result *(*p1)(parse_state *ps1), result *(*p2)(parse_state *ps2)))(parse_state *ps) { return NULL; }
result* epsilon_p(parse_state *ps) { return NULL; }
//result (*semantic(/* fptr to nullary function? */))(parse_state *ps) { return NULL; }
result* (*and(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
result* (*not(result *(*p1)(parse_state *ps1)))(parse_state *ps) { return NULL; }
