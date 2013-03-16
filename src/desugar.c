
HCFChoice *h_desugar(HAllocator *mm__, HParser *parser) {
  return parser->vtable->desugar(mm__, parser);
}
