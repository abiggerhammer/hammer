#include "parser_internal.h"

struct bits_env {
  uint8_t length;
  uint8_t signedp;
};

static HParseResult* parse_bits(void* env, HParseState *state) {
  struct bits_env *env_ = env;
  HParsedToken *result = a_new(HParsedToken, 1);
  result->token_type = (env_->signedp ? TT_SINT : TT_UINT);
  if (env_->signedp)
    result->sint = h_read_bits(&state->input_stream, env_->length, true);
  else
    result->uint = h_read_bits(&state->input_stream, env_->length, false);
  return make_result(state, result);
}

static const HParserVtable bits_vt = {
  .parse = parse_bits,
  .isValidRegular = h_true,
  .isValidCF = h_true,
};
const HParser* h_bits(size_t len, bool sign) {
  return h_bits__m(&system_allocator, len, sign);
}
const HParser* h_bits__m(HAllocator* mm__, size_t len, bool sign) {
  struct bits_env *env = h_new(struct bits_env, 1);
  env->length = len;
  env->signedp = sign;
  HParser *res = h_new(HParser, 1);
  res->vtable = &bits_vt;
  res->env = env;
  return res;
}

#define SIZED_BITS(name_pre, len, signedp) \
  const HParser* h_##name_pre##len () {				\
    return h_bits__m(&system_allocator, len, signedp);		\
  }								\
  const HParser* h_##name_pre##len##__m(HAllocator* mm__) {	\
    return h_bits__m(mm__, len, signedp);			\
  }
SIZED_BITS(int, 8, true)
SIZED_BITS(int, 16, true)
SIZED_BITS(int, 32, true)
SIZED_BITS(int, 64, true)
SIZED_BITS(uint, 8, false)
SIZED_BITS(uint, 16, false)
SIZED_BITS(uint, 32, false)
SIZED_BITS(uint, 64, false)
