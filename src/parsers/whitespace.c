#include <ctype.h>
#include "parser_internal.h"

static HParseResult* parse_whitespace(void* env, HParseState *state) {
  char c;
  HInputStream bak;
  do {
    bak = state->input_stream;
    c = h_read_bits(&state->input_stream, 8, false);
    if (state->input_stream.overrun)
      return NULL;
  } while (isspace(c));
  state->input_stream = bak;
  return h_do_parse((HParser*)env, state);
}

static HCFChoice* desugar_whitespace(HAllocator *mm__, void *env) {
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 3);
  HCFSequence *nonempty = h_new(HCFSequence, 1);
  nonempty->items = h_new(HCFChoice*, 3);
  nonempty->items[0] = h_new(HCFChoice, 1);
  nonempty->items[0]->type = HCF_CHARSET;
  nonempty->items[0]->charset = new_charset(mm__);
  charset_set(nonempty->items[0]->charset, '\t', 1);
  charset_set(nonempty->items[0]->charset, ' ', 1);
  charset_set(nonempty->items[0]->charset, '\n', 1);
  charset_set(nonempty->items[0]->charset, '\r', 1);
  nonempty->items[1] = ret; // yay circular pointer!
  nonempty->items[2] = NULL;
  ret->seq[0] = nonempty;
  HCFSequence *empty = h_new(HCFSequence, 1);
  empty->items = h_new(HCFChoice*, 1);
  empty->items[0] = NULL;
  ret->seq[1] = empty;
  ret->seq[2] = NULL;
  ret->action = NULL;
  return ret;
}

static bool ws_isValidRegular(void *env) {
  HParser *p = (HParser*)env;
  return p->vtable->isValidRegular(p->env);
}

static bool ws_isValidCF(void *env) {
  HParser *p = (HParser*)env;
  return p->vtable->isValidCF(p->env);
}

static bool ws_ctrvm(HRVMProg *prog, void *env) {
  HParser *p = (HParser*)env;
  uint16_t start = h_rvm_get_ip(prog);
  uint16_t next;
  const char SPACE_CHRS[6] = {' ', '\f', '\n', '\r', '\t', '\v'};

  for (int i = 0; i < 6; i++) {
    next = h_rvm_insert_insn(prog, RVM_FORK, 0);
    h_rvm_insert_insn(prog, RVM_MATCH, (SPACE_CHRS[i] << 8) | (SPACE_CHRS[i]));
    h_rvm_insert_insn(prog, RVM_GOTO, start);
    h_rvm_patch_arg(prog, next, h_rvm_get_ip(prog));
  }
  return h_compile_regex(prog, p->env);
}

static const HParserVtable whitespace_vt = {
  .parse = parse_whitespace,
  .isValidRegular = ws_isValidRegular,
  .isValidCF = ws_isValidCF,
  .desugar = desugar_whitespace,
  .compile_to_rvm = ws_ctrvm,
};

HParser* h_whitespace(const HParser* p) {
  return h_whitespace__m(&system_allocator, p);
}
HParser* h_whitespace__m(HAllocator* mm__, const HParser* p) {
  return h_new_parser(mm__, &whitespace_vt, (void *)p);
}
