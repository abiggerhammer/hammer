#include "hammer.h"
#include "internal.h"

HCFChoice *h_desugar(HAllocator *mm__, const HParser *parser) {
  return parser->vtable->desugar(mm__, parser->env);
}
