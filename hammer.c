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
