#include "hammer.h"
#include "internal.h"
#include "backends/contextfree.h"

HCFChoice *h_desugar(HAllocator *mm__, HCFStack *stk__, const HParser *parser) {
  HCFStack *nstk__ = stk__;
  if(parser->desugared == NULL) {
    if (nstk__ == NULL) {
      nstk__ = h_cfstack_new(mm__);
    }
    // we're going to do something naughty and cast away the const to memoize
    parser->vtable->desugar(mm__, nstk__, parser->env);
    ((HParser *)parser)->desugared = nstk__->last_completed;
    if (stk__ == NULL)
      h_cfstack_free(mm__, nstk__);
  } else if (stk__ != NULL) {
    HCFS_APPEND(parser->desugared);
  }

  return parser->desugared;
}
