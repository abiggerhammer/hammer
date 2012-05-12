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
