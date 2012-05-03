#ifndef HAMMER_INTERNAL__H
#define HAMMER_INTERNAL__H
#include "hammer.h"

#define false 0
#define true 1

typedef struct parser_cache_key {
  input_stream_t input_pos;
  const parser_t *parser;
} parser_cache_key_t;

// TODO(thequux): Set symbol visibility for these functions so that they aren't exported.

long long read_bits(input_stream_t* state, int count, char signed_p);
parse_result_t* do_parse(const parser_t* parser, parse_state_t *state);
void put_cached(parse_state_t *ps, const parser_t *p, parse_result_t *cached);
guint djbhash(const uint8_t *buf, size_t len);
#endif // #ifndef HAMMER_INTERNAL__H
