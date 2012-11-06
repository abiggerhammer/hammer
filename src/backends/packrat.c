#include "../internal.h"

int h_packrat_compile(HAllocator* mm__, const HParser* parser, const void* params) {
  return 0; // No compilation necessary, and everything should work
	    // out of the box.
}

HParseResult *h_packrat_parse(HAllocator* mm__, const HParser* parser, HParseState* parse_state) {
  return NULL; // TODO: fill this in.
}

HParserBackendVTable h__packrat_backend_vtable = {
  .compile = h_packrat_compile,
  .parse = h_packrat_parse
};
