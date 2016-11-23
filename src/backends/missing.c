#include "missing.h"

int h_missing_compile(HAllocator* mm__, HParser* parser, const void* params) {
  /* Always fail */

  return -1;
}

HParseResult *h_missing_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream) {
  /* Always fail */

  return NULL;
}

void h_missing_free(HParser *parser) {
  /* No-op */
}

HParserBackendVTable h__missing_backend_vtable = {
  .compile = h_missing_compile, /* TODO */
  .parse = h_missing_parse, /* TODO */
  .free = h_missing_free, /* TODO */
};
