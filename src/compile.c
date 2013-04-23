// This file contains functions related to managing multiple parse backends
#include "hammer.h"
#include "internal.h"

static HParserBackendVTable *backends[PB_MAX] = {
  &h__packrat_backend_vtable,
};

int h_compile(const HParser* parser, HParserBackend backend, const void* params) {
  return h_compile__m(&system_allocator, parser, backend, params);
}

int h_compile__m(HAllocator* mm__, const HParser* parser, HParserBackend backend, const void* params) {
  return backends[backend]->compile(mm__, parser, params);
}
