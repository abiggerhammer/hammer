#include <assert.h>
#include "../internal.h"
#include "../parsers/parser_internal.h"


int h_ll_compile(HAllocator* mm__, const HParser* parser, const void* params) {
  return -1; // TODO
}

HParseResult *h_ll_parse(HAllocator* mm__, const HParser* parser, HParseState* parse_state) {
  return NULL; // TODO
}

HParserBackendVTable h__ll_backend_vtable = {
  .compile = h_ll_compile,
  .parse = h_ll_parse
};
