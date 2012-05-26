#include "parser_internal.h"

static HParseResult* parse_end(void *env, HParseState *state) {
  if (state->input_stream.index == state->input_stream.length) {
    HParseResult *ret = a_new(HParseResult, 1);
    ret->ast = NULL;
    return ret;
  } else {
    return NULL;
  }
}

static const HParserVtable end_vt = {
  .parse = parse_end,
};

const HParser* h_end_p() { 
  HParser *ret = g_new(HParser, 1);
  ret->vtable = &end_vt; ret->env = NULL;
  return (const HParser*)ret;
}
