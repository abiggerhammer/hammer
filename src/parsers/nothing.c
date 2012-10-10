#include "parser_internal.h"


static HParseResult* parse_nothing() {
  // not a mistake, this parser always fails
  return NULL;
}

static const HParserVtable nothing_vt = {
  .parse = parse_nothing,
};

const HParser* h_nothing_p() {
  return h_nothing_p__m(&system_allocator);
}
const HParser* h_nothing_p__m(HAllocator* mm__) { 
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &nothing_vt; ret->env = NULL;
  return (const HParser*)ret;
}
