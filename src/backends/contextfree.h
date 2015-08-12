/*
 * NOTE: This is an internal header and installed for use by extensions. The
 * API is not guaranteed stable.
*/

// This is an internal header; it provides macros to make desugaring cleaner.
#include <assert.h>
#include "../internal.h"
#ifndef HAMMER_CONTEXTFREE__H
#define HAMMER_CONTEXTFREE__H


// HCFStack
struct HCFStack_ {
  HCFChoice **stack;
  int count;
  int cap;
  HCFChoice *last_completed; // Last completed choice.
                             // XXX is last_completed still needed?
  HCFChoice *prealloc; // If not NULL, will be used for the outermost choice.
  char error;
};

#ifndef UNUSED
#define UNUSED H_GCC_ATTRIBUTE((unused))
#endif

static inline HCFChoice* h_cfstack_new_choice_raw(HAllocator *mm__, HCFStack *stk__) UNUSED;
static inline void h_cfstack_begin_choice(HAllocator *mm__, HCFStack *stk__) UNUSED;
static HCFStack* h_cfstack_new(HAllocator *mm__) UNUSED; 
static HCFStack* h_cfstack_new(HAllocator *mm__) {
  HCFStack *stack = h_new(HCFStack, 1);
  stack->count = 0;
  stack->cap = 4;
  stack->stack = h_new(HCFChoice*, stack->cap);
  stack->prealloc = NULL;
  stack->error = 0;
  return stack;
}

static void h_cfstack_free(HAllocator *mm__, HCFStack *stk__) UNUSED; 
static void h_cfstack_free(HAllocator *mm__, HCFStack *stk__) {
  h_free(stk__->prealloc);
  h_free(stk__->stack);
  h_free(stk__);
}

static inline void h_cfstack_add_to_seq(HAllocator *mm__, HCFStack *stk__, HCFChoice *item) UNUSED;
static inline void h_cfstack_add_to_seq(HAllocator *mm__, HCFStack *stk__, HCFChoice *item) {
  HCFChoice *cur_top = stk__->stack[stk__->count-1];
  assert(cur_top->type == HCF_CHOICE);
  assert(cur_top->seq[0] != NULL); // There must be at least one sequence...
  stk__->last_completed = item;
  for (int i = 0;; i++) {
    if (cur_top->seq[i+1] == NULL) {
      assert(cur_top->seq[i]->items != NULL);
      for (int j = 0;; j++) {
	if (cur_top->seq[i]->items[j] == NULL) {
	  cur_top->seq[i]->items = mm__->realloc(mm__, cur_top->seq[i]->items, sizeof(HCFChoice*) * (j+2));
	  if (!cur_top->seq[i]->items) {
	    stk__->error = 1;
	  }
	  cur_top->seq[i]->items[j] = item;
	  cur_top->seq[i]->items[j+1] = NULL;
	  assert(!stk__->error);
	  return;
	}
      }
    }
  }
}

static inline HCFChoice* h_cfstack_new_choice_raw(HAllocator *mm__, HCFStack *stk__) {
  HCFChoice *ret = stk__->prealloc? stk__->prealloc : h_new(HCFChoice, 1);
  stk__->prealloc = NULL;

  ret->reshape = NULL;
  ret->action = NULL;
  ret->pred = NULL;
  ret->type = ~0; // invalid type
  // Add it to the current sequence...
  if (stk__->count > 0) {
    h_cfstack_add_to_seq(mm__, stk__, ret);
  }

  return ret;
}

static inline void h_cfstack_add_charset(HAllocator *mm__, HCFStack *stk__, HCharset charset) {
  HCFChoice *ni = h_cfstack_new_choice_raw(mm__, stk__);
  ni->type = HCF_CHARSET;
  ni->charset = charset;
  stk__->last_completed = ni;
}


static inline void h_cfstack_add_char(HAllocator *mm__, HCFStack *stk__, uint8_t chr) {
  HCFChoice *ni = h_cfstack_new_choice_raw(mm__, stk__);
  ni->type = HCF_CHAR;
  ni->chr = chr;
  stk__->last_completed = ni;
}

static inline void h_cfstack_add_end(HAllocator *mm__, HCFStack *stk__) {
  HCFChoice *ni = h_cfstack_new_choice_raw(mm__, stk__);
  ni->type = HCF_END;
  stk__->last_completed = ni;
}

static inline void h_cfstack_begin_choice(HAllocator *mm__, HCFStack *stk__) {
  HCFChoice *choice = h_cfstack_new_choice_raw(mm__, stk__);
  choice->type = HCF_CHOICE;
  choice->seq = h_new(HCFSequence*, 1);
  choice->seq[0] = NULL;

  if (stk__->count + 1 > stk__->cap) {
    assert(stk__->cap > 0);
    stk__->cap *= 2;
    stk__->stack = mm__->realloc(mm__, stk__->stack, stk__->cap * sizeof(HCFChoice*));
    if (!stk__->stack) {
      stk__->error = 1;
    }
  }
  assert(stk__->cap >= 1 && !stk__->error);
  stk__->stack[stk__->count++] = choice;
}

static inline void h_cfstack_begin_seq(HAllocator *mm__, HCFStack *stk__) {
  HCFChoice *top = stk__->stack[stk__->count-1];
  for (int i = 0;; i++) {
    if (top->seq[i] == NULL) {
      top->seq = mm__->realloc(mm__, top->seq, sizeof(HCFSequence*) * (i+2));
      if (!top->seq) {
	stk__->error = 1;
	return;
      }
      HCFSequence *seq = top->seq[i] = h_new(HCFSequence, 1);
      top->seq[i+1] = NULL;
      seq->items = h_new(HCFChoice*, 1);
      seq->items[0] = NULL;
      return;
    }
  }
}

static inline void h_cfstack_end_seq(HAllocator *mm__, HCFStack *stk__) UNUSED;
static inline void h_cfstack_end_seq(HAllocator *mm__, HCFStack *stk__) {
  // do nothing. You should call this anyway.
}

static inline void h_cfstack_end_choice(HAllocator *mm__, HCFStack *stk__) UNUSED;
static inline void h_cfstack_end_choice(HAllocator *mm__, HCFStack *stk__) {
  assert(stk__->count > 0);
  stk__->last_completed = stk__->stack[stk__->count-1];
  stk__->count--;
}

#define HCFS_APPEND(choice) h_cfstack_add_to_seq(mm__, stk__, (choice))
#define HCFS_DESUGAR(parser) h_desugar(mm__, stk__, parser)
#define HCFS_ADD_CHARSET(charset) h_cfstack_add_charset(mm__, stk__, (charset))
#define HCFS_ADD_CHAR(chr) h_cfstack_add_char(mm__, stk__, (chr))
#define HCFS_ADD_END() h_cfstack_add_end(mm__, stk__)
// The semicolons on BEGIN macros are intentional; pretend that they
// are control structures.
#define HCFS_BEGIN_CHOICE() h_cfstack_begin_choice(mm__, stk__);
#define HCFS_BEGIN_SEQ() h_cfstack_begin_seq(mm__, stk__);
#define HCFS_END_CHOICE() h_cfstack_end_choice(mm__, stk__)
#define HCFS_END_SEQ() h_cfstack_end_seq(mm__, stk__)
#define HCFS_THIS_CHOICE (stk__->stack[stk__->count-1])

#endif
