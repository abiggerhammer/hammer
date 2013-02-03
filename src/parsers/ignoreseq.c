#include "parser_internal.h"


//
// general case: parse sequence, pick one result
//

typedef struct {
  const HParser **parsers;
  size_t len;         // how many parsers in 'ps'
  size_t which;         // whose result to return
} HIgnoreSeq;

static HParseResult* parse_ignoreseq(void* env, HParseState *state) {
  const HIgnoreSeq *seq = (HIgnoreSeq*)env;
  HParseResult *res = NULL;

  for (size_t i=0; i < seq->len; ++i) {
    HParseResult *tmp = h_do_parse(seq->parsers[i], state);
    if (!tmp)
      return NULL;
    else if (i == seq->which)
      res = tmp;
  }

  return res;
}

static HCFChoice* desugar_ignoreseq(HAllocator *mm__, void *env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  HCFSequence *hseq = h_new(HCFSequence, 1);
  hseq->items = h_new(HCFChoice*, 1+seq->len);
  for (size_t i=0; i<seq->len; ++i) {
    hseq->items[i] = seq->parsers[i]->vtable->desugar(mm__, seq->parsers[i]->env);
  }
  hseq->items[seq->len] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = hseq;
  ret->seq[1] = NULL;
  ret->action = NULL;
  return ret;
}

static bool is_isValidRegular(void *env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  for (size_t i=0; i<seq->len; ++i) {
    if (!seq->parsers[i]->vtable->isValidRegular(seq->parsers[i]->env))
      return false;
  }
  return true;
}

static bool is_isValidCF(void *env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  for (size_t i=0; i<seq->len; ++i) {
    if (!seq->parsers[i]->vtable->isValidCF(seq->parsers[i]->env))
      return false;
  }
  return true;
}

static const HParserVtable ignoreseq_vt = {
  .parse = parse_ignoreseq,
  .isValidRegular = is_isValidRegular,
  .isValidCF = is_isValidCF,
  .desugar = desugar_ignoreseq,
};


//
// API frontends
//

static const HParser* h_leftright__m(HAllocator* mm__, const HParser* p, const HParser* q, size_t which) {
  HIgnoreSeq *seq = h_new(HIgnoreSeq, 1);
  seq->parsers = h_new(const HParser*, 2);
  seq->parsers[0] = p;
  seq->parsers[1] = q;
  seq->len = 2;
  seq->which = which;

  HParser *ret = h_new(HParser, 1);
  ret->vtable = &ignoreseq_vt;
  ret->env = (void*)seq;
  return ret;
}

const HParser* h_left(const HParser* p, const HParser* q) {
  return h_leftright__m(&system_allocator, p, q, 0);
}
const HParser* h_left__m(HAllocator* mm__, const HParser* p, const HParser* q) {
  return h_leftright__m(mm__, p, q, 0);
}

const HParser* h_right(const HParser* p, const HParser* q) {
  return h_leftright__m(&system_allocator, p, q, 1);
}
const HParser* h_right__m(HAllocator* mm__, const HParser* p, const HParser* q) {
  return h_leftright__m(mm__, p, q, 1);
}


const HParser* h_middle(const HParser* p, const HParser* x, const HParser* q) {
  return h_middle__m(&system_allocator, p, x, q);
}
const HParser* h_middle__m(HAllocator* mm__, const HParser* p, const HParser* x, const HParser* q) {
  HIgnoreSeq *seq = h_new(HIgnoreSeq, 1);
  seq->parsers = h_new(const HParser*, 3);
  seq->parsers[0] = p;
  seq->parsers[1] = x;
  seq->parsers[2] = q;
  seq->len = 3;
  seq->which = 1;

  HParser *ret = h_new(HParser, 1);
  ret->vtable = &ignoreseq_vt;
  ret->env = (void*)seq;
  return ret;
}
