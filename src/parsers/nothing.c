#include "parser_internal.h"


static HParseResult* parse_nothing() {
  // not a mistake, this parser always fails
  return NULL;
}

static const HParserVtable nothing_vt = {
  .parse = parse_nothing,
};

const HParser* h_nothing_p() { 
  HParser *ret = g_new(HParser, 1);
  ret->vtable = &nothing_vt; ret->env = NULL;
  return (const HParser*)ret;
}
