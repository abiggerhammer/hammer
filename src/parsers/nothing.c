#include "parser_internal.h"


static HParseResult* parse_nothing() {
  // not a mistake, this parser always fails
  return NULL;
}

static HCFChoice *desugar_nothing(HAllocator *mm__, void *env) {
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 1);
  ret->seq[0] = NULL;
  ret->action = NULL;
  return ret;
}

static const HParserVtable nothing_vt = {
  .parse = parse_nothing,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_nothing,
};

const HParser* h_nothing_p() {
  return h_nothing_p__m(&system_allocator);
}
const HParser* h_nothing_p__m(HAllocator* mm__) { 
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &nothing_vt; ret->env = NULL;
  return (const HParser*)ret;
}
