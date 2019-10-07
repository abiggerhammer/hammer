#include <ctype.h>
#include <assert.h>
#include "parser_internal.h"

static HParseResult* parse_whitespace(void* env, HParseState *state) {
  char c;
  HInputStream bak;
  do {
    bak = state->input_stream;
    c = h_read_bits(&state->input_stream, 8, false);
    if (state->input_stream.overrun)
      break;
  } while (isspace((int)c));
  state->input_stream = bak;
  return h_do_parse((HParser*)env, state);
}

static const char SPACE_CHRS[6] = {' ', '\f', '\n', '\r', '\t', '\v'};

static void desugar_whitespace(HAllocator *mm__, HCFStack *stk__, void *env) {

  HCharset ws_cs = new_charset(mm__);
  for(size_t i=0; i<sizeof(SPACE_CHRS); i++)
    charset_set(ws_cs, SPACE_CHRS[i], 1);
  
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      HCFS_BEGIN_CHOICE() {
	HCFS_BEGIN_SEQ() {
	  HCFS_ADD_CHARSET(ws_cs);
	  HCFS_APPEND(HCFS_THIS_CHOICE); // yay circular pointer!
	} HCFS_END_SEQ();
	HCFS_BEGIN_SEQ() {
	} HCFS_END_SEQ();
      } HCFS_END_CHOICE();
      HCFS_DESUGAR( (HParser*)env );
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = h_act_last;
  } HCFS_END_CHOICE();
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

  uint16_t ranges[2] = {
    0x0d09,
    0x2020,
  };
  
  for (int i = 0; i < 2; i++) {
    next = h_rvm_insert_insn(prog, RVM_FORK, 0);
    h_rvm_insert_insn(prog, RVM_MATCH, ranges[i]);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
    h_rvm_insert_insn(prog, RVM_GOTO, start);
    h_rvm_patch_arg(prog, next, h_rvm_get_ip(prog));
  }
  return h_compile_regex(prog, p);
}

static const HParserVtable whitespace_vt = {
  .parse = parse_whitespace,
  .isValidRegular = ws_isValidRegular,
  .isValidCF = ws_isValidCF,
  .desugar = desugar_whitespace,
  .compile_to_rvm = ws_ctrvm,
  .higher = false,
};

HParser* h_whitespace(const HParser* p) {
  return h_whitespace__m(&system_allocator, p);
}
HParser* h_whitespace__m(HAllocator* mm__, const HParser* p) {
  void* env = (void*)p;
  return h_new_parser(mm__, &whitespace_vt, env);
}
