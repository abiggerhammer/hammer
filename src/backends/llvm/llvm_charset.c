#ifdef HAMMER_LLVM_BACKEND

#include <llvm-c/Analysis.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <llvm-c/Core.h>
#pragma GCC diagnostic pop
#include <llvm-c/ExecutionEngine.h>
#include "../../internal.h"
#include "llvm.h"

/*
 * Set this #define to enable some debug logging and internal consistency
 * checking.
 */
#define HAMMER_LLVM_CHARSET_DEBUG

typedef enum {
  /*
   * Accept action; this entire range is in the charset.  This action type
   * has no children and terminates handling the input character.
   */
  CHARSET_ACTION_ACCEPT,
  /*
   * Scan action; test input char against each set character in the charset.
   * This action type has no children and terminates handling the input
   * character.
   */
  CHARSET_ACTION_SCAN,
  /*
   * Bitmap action; test input char against a bitmap in the IR at fixed
   * cost.
   */
  CHARSET_ACTION_BITMAP,
  /*
   * Complement action; invert the sense of the charset.  This action type
   * has one child node, with the bounds unchanged and the portion of the
   * charset within the bounds complemented.
   */
  CHARSET_ACTION_COMPLEMENT,
  /*
   * Split action; check whether the input char is above or below a split
   * point, and branch into one of two children depending.
   */
  CHARSET_ACTION_SPLIT
} llvm_charset_exec_plan_action_t;

typedef struct llvm_charset_exec_plan_s llvm_charset_exec_plan_t;
struct llvm_charset_exec_plan_s {
  /*
   * The charset at this node, with transforms such as range restriction
   * or complementation applied.
   */
  HCharset cs;
  /*
   * Char values for the range of this node, and the split point if this
   * is CHARSET_ACTION_SPLIT
   */
  uint8_t idx_start, idx_end, split_point;
  /* Action to take at this node */
  llvm_charset_exec_plan_action_t action;
  /* Estimated cost metric */
  int cost;
  /* Depth in exec plan */
  int depth;
  /* Children, if any (zero, one or two depending on action) */
  llvm_charset_exec_plan_t *children[2];
};

/* Forward prototypes for charset llvm stuff */
static int h_llvm_build_charset_exec_plan_impl(HAllocator* mm__, HCharset cs,
    llvm_charset_exec_plan_t *parent, llvm_charset_exec_plan_t *cep,
    int allow_complement, uint8_t *split_point);
static llvm_charset_exec_plan_t * h_llvm_build_charset_exec_plan_impl_alloc(
    HAllocator* mm__, llvm_charset_exec_plan_t *parent, HCharset cs,
    uint8_t idx_start, uint8_t idx_end, int allow_complement);
static void h_llvm_free_charset_exec_plan(HAllocator* mm__,
                                          llvm_charset_exec_plan_t *cep);

/*
 * Check if this charset is eligible for CHARSET_ACTION_ACCEPT on a range
 */

static int h_llvm_charset_eligible_for_accept(HCharset cs, uint8_t idx_start, uint8_t idx_end) {
  int eligible = 1, i;

  for (i = idx_start; i <= idx_end; ++i) {
    if (!(charset_isset(cs, (uint8_t)i))) {
      eligible = 0;
      break;
    }
  }

  return eligible;
}

/*
 * Estimate cost of CHARSET_ACTION_SCAN for this charset (~proportional to number of set chars, min 1)
 */

static int h_llvm_charset_estimate_scan_cost(HCharset cs, uint8_t idx_start, uint8_t idx_end) {
  int i, cost;

  cost = 1;
  for (i = idx_start; i <= idx_end; ++i) {
    if (charset_isset(cs, (uint8_t)i)) ++cost;
  }

  return cost;
}

/*
 * Given a skeletal CHARSET_ACTION_SPLIT node from h_llvm_build_charset_exec_plan_impl(),
 * binary search for the best split point we can find and return the cost metric.
 * Unfortunately the search space is quite large, so we're going to use some silly
 * heuristics here such as looking for the longest run of present or absent chars at
 * one end of a charset, and proposing it as a split, or just trying the midpoint.
 * It may be possible to do better.
 */

static int h_llvm_find_best_split(HAllocator* mm__, llvm_charset_exec_plan_t *split) {
  int rv, best_end_run, i, contiguous;
  uint8_t best_end_run_split, midpoint;
  llvm_charset_exec_plan_t *best_left, *best_right, *left, *right;
  int best_cost, cost;

  /* Sanity-check: we should be a split with a range at least two indices long */
  if (!split || split->action != CHARSET_ACTION_SPLIT) return -1;
  if (split->idx_end <= split->idx_start) return -1;

  /* Find the longest end run; split a run of length 1 at the left end as a
   * fallback, since there's always a run of length 1 at each end. */
  best_end_run = 1;
  best_end_run_split = split->idx_start;
  contiguous = 0;
  /* Try the low end */
  i = 0;
  while (i <= split->idx_end - split->idx_start &&
         (charset_isset(split->cs, split->idx_start + i) ==
          charset_isset(split->cs, split->idx_start))) ++i;
  if (i <= split->idx_end - split->idx_start) {
    /* This run has length i */
    if (i > best_end_run) {
      best_end_run = i;
      /*
       * -1 since split points are last index of left child, and i
       * is first index that wasn't in the run
       */
      best_end_run_split = split->idx_start + i - 1;
    }

    /* Now the same thing from the high end */
    i = 0;
    while (i <= split->idx_end - split->idx_start &&
           (charset_isset(split->cs, split->idx_end - i) ==
            charset_isset(split->cs, split->idx_end))) ++i;
    if (i <= split->idx_end - split->idx_start && i > best_end_run) {
      best_end_run = i;
      best_end_run_split = split->idx_end - i;
    }
  } else {
    /* Wow, contiguous - any split will turn out well - just use the midpoint */
    contiguous = 1;
  }

  /* Initialize, start trying things */
  best_left = best_right = left = right = NULL;
  rv = -1;

  /* Try a midpoint split */
  midpoint = split->idx_start + (split->idx_end - split->idx_start) / 2;
  left = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
      split->idx_start, midpoint, 1);
  right = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
      midpoint + 1, split->idx_end, 1);
  if (left && right) {
    /* Cost of the split == 1 + max(left->cost, right->cost) */
    cost = left->cost;
    if (right->cost > cost) cost = right->cost;
    ++cost;
    /* We haven't tried the end-run one yet, so always accept this */
    best_left = left;
    best_right = right;
    best_cost = cost;
    left = right = NULL;
  } else goto err;

  /*
   * Try an end-run split; if we decided we had a contiguous run earlier,
   * all are equally good, so don't bother and just use the midpoint
   */

  if (!contiguous) {
    /*
     * Sanity-check the indices; error out if the scanner gave us
     * something silly
     */
    if (best_end_run_split < split->idx_start ||
        best_end_run_split >= split->idx_end) goto err;
    left = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
        split->idx_start, best_end_run_split, 1);
    right = h_llvm_build_charset_exec_plan_impl_alloc(mm__, split, split->cs,
       best_end_run_split + 1, split->idx_end, 1);
    if (left && right) {
      /* Cost of the split == 1 + max(left->cost, right->cost) */
      cost = left->cost;
      if (right->cost > cost) cost = right->cost;
      ++cost;
      /* Check if against what we already have */
      if (cost < best_cost) {
        if (best_left) h_llvm_free_charset_exec_plan(mm__, best_left);
        if (best_right) h_llvm_free_charset_exec_plan(mm__, best_right);
        best_left = left;
        best_right = right;
        best_cost = cost;
        left = right = NULL;
      }
    } else goto err;
  }

  /* Set up the split node with our best results */
  split->cost = best_cost;
  split->children[0] = best_left;
  split->children[1] = best_right;
  split->split_point = best_left->idx_end;
  best_left = best_right = NULL;
  rv = split->cost;

 err:
  /* Error/cleanup case */
  if (left) h_llvm_free_charset_exec_plan(mm__, left);
  if (right) h_llvm_free_charset_exec_plan(mm__, right);
  if (best_left) h_llvm_free_charset_exec_plan(mm__, best_left);
  if (best_right) h_llvm_free_charset_exec_plan(mm__, best_right);

  return rv;
}

/*
 * Setup call to h_llvm_build_charset_exec_plan_impl(), while allocating a new
 * llvm_charset_exec_plan_t.
 */
static llvm_charset_exec_plan_t * h_llvm_build_charset_exec_plan_impl_alloc(
    HAllocator* mm__, llvm_charset_exec_plan_t *parent, HCharset cs,
    uint8_t idx_start, uint8_t idx_end, int allow_complement) {
  int cost;
  llvm_charset_exec_plan_t *cep;

  if (!mm__) return NULL;
  if (!cs) return NULL;
  if (idx_start > idx_end) return NULL;

  cep = h_new(llvm_charset_exec_plan_t, 1);
  memset(cep, 0, sizeof(*cep));
  cep->cs = NULL;
  /*
   * Initializing these is important; if the parent is CHARSET_ACTION_SPLIT,
   * these are how h_llvm_build_charset_exec_plan_impl() knows the range for
   * the child it's constructing.
   */
  cep->idx_start = idx_start;
  cep->idx_end = idx_end;
  cost = h_llvm_build_charset_exec_plan_impl(mm__, cs, parent, cep,
      allow_complement, NULL);
  if (cost >= 0) cep->cost = cost;
  else {
    h_llvm_free_charset_exec_plan(mm__, cep);
    cep = NULL;
  }

  return cep;
}

/*
 * Given a charset, optionally its parent containing range restrictions, and
 * an allow_complement parameter, search for the best exec plan and write it
 * to another (skeletal) charset which will receive an action and range.  If
 * the action is CHARSET_ACTION_SPLIT, also output a split point.  Return a
 * cost estimate.
 */

static int h_llvm_build_charset_exec_plan_impl(HAllocator* mm__, HCharset cs,
    llvm_charset_exec_plan_t *parent, llvm_charset_exec_plan_t *cep,
    int allow_complement, uint8_t *split_point) {
  int eligible_for_accept, best_cost, depth;
  int estimated_complement_cost, estimated_scan_cost, estimated_split_cost;
  int estimated_bitmap_cost;
  uint8_t idx_start, idx_end;
  llvm_charset_exec_plan_t complement_cep, split_cep;
  llvm_charset_exec_plan_action_t chosen_action;

  /* Check args */
  if (!(mm__ && cep)) return -1;

  /*
   * The index bounds come from either the parent or maximal bounds by
   * default.  Exception is the case that we are a child of a split, in
   * which case h_llvm_find_best_split() should have set bounds in cep.
   */
  if (parent && parent->action == CHARSET_ACTION_SPLIT &&
      ((cep->idx_start == parent->idx_start &&
        cep->idx_end < parent->idx_end) ||
       (cep->idx_start > parent->idx_start &&
        cep->idx_end == parent->idx_end))) {
    idx_start = cep->idx_start;
    idx_end = cep->idx_end;
  } else if (parent) {
    idx_start = parent->idx_start;
    idx_end = parent->idx_end;
  } else {
    idx_start = 0;
    idx_end = UINT8_MAX;
  }

  /* Get the depth in the exec plan */
  if (parent) depth = parent->depth + 1;
  else depth = 0;

  eligible_for_accept = h_llvm_charset_eligible_for_accept(cs, idx_start, idx_end);
  if (eligible_for_accept) {
    /* if we can use CHARSET_ACTION_ACCEPT, always do so */
    cep->cs = copy_charset(mm__, cs);
    charset_restrict_to_range(cep->cs, idx_start, idx_end);
    cep->idx_start = idx_start;
    cep->idx_end = idx_end;
    cep->split_point = 0;
    /* Acceptance (or rejection, under a complement) is free */
    cep->cost = 0;
    cep->depth = depth;
    cep->action = CHARSET_ACTION_ACCEPT;
    cep->children[0] = NULL;
    cep->children[1] = NULL;

    return cep->cost;
  } else {
    /*
     * Estimate cost for CHARSET_ACTION_SCAN, and for the tree below
     * CHARSET_ACTION_COMPLEMENT if we are eligible to use it.
     */
    estimated_scan_cost = h_llvm_charset_estimate_scan_cost(cs, idx_start, idx_end);
    /*
     * We can always use CHARSET_ACTION_BITMAP; this constant controls how
     * strongly we prefer it over the compare-and-branch approach.
     */
    estimated_bitmap_cost = 6;
    /* >= 0 is a flag we have a complement we may need to free later */
    estimated_complement_cost = -1;
    if (allow_complement) {
      HCharset child_cs;

      /* Complement the charset within the range */
      memset(&complement_cep, 0, sizeof(complement_cep));
      complement_cep.cs = copy_charset(mm__, cs);
      charset_restrict_to_range(complement_cep.cs, idx_start, idx_end);
      child_cs = copy_charset(mm__, complement_cep.cs);
      charset_complement(child_cs);
      charset_restrict_to_range(child_cs, idx_start, idx_end);
      complement_cep.idx_start = idx_start;
      complement_cep.idx_end = idx_end;
      complement_cep.split_point = 0;
      complement_cep.depth = depth;
      complement_cep.action = CHARSET_ACTION_COMPLEMENT;
      complement_cep.children[0] = h_new(llvm_charset_exec_plan_t, 1);
      memset(complement_cep.children[0], 0, sizeof(llvm_charset_exec_plan_t));
      complement_cep.children[1] = NULL;
      /*
       * Find the child; the complement has cost 0 since it just swizzles success
       * and fail output basic blocks; it's important we test for complement last
       * below then, so we break ties in favor of not stacking complements up.  We
       * set allow_complement = 0 so we never stack two complements.
       */
      complement_cep.cost = h_llvm_build_charset_exec_plan_impl(mm__, child_cs, &complement_cep,
          complement_cep.children[0], 0, NULL);
      estimated_complement_cost = complement_cep.cost;
      h_free(child_cs);
    }

    /*
     * Set up split node if it makes sense; the depth cutoff here limits the
     * cost of the search for complex charsets.
     */
    if (idx_start < idx_end && depth < 5) {
      split_cep.cs = copy_charset(mm__, cs);
      charset_restrict_to_range(split_cep.cs, idx_start, idx_end);
      split_cep.idx_start = idx_start;
      split_cep.idx_end = idx_end;
      split_cep.split_point = 0;
      split_cep.action = CHARSET_ACTION_SPLIT;
      split_cep.cost = -1;
      split_cep.depth = depth;
      split_cep.children[0] = NULL;
      split_cep.children[1] = NULL;
      /* h_llvm_find_best_split() sets split_cep.cost */
      estimated_split_cost = h_llvm_find_best_split(mm__, &split_cep);
      if (estimated_split_cost < 0) {
        /* This shouldn't happen, but make sure we free the charset */
        h_free(split_cep.cs);
      }
    } else {
      estimated_split_cost = -1;
    }

    /* Pick the action type with the lowest cost */
    best_cost = -1;
    if (estimated_scan_cost >= 0 &&
        (best_cost < 0 || estimated_scan_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_SCAN;
      best_cost = estimated_scan_cost;
    }

    if (estimated_bitmap_cost >= 0 &&
        (best_cost < 0 || estimated_bitmap_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_BITMAP;
      best_cost = estimated_bitmap_cost;
    }

    if (estimated_split_cost >= 0 &&
        (best_cost < 0 || estimated_split_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_SPLIT;
      best_cost = estimated_split_cost;
    }

    if (allow_complement && estimated_complement_cost >= 0 &&
        (best_cost < 0 || estimated_complement_cost < best_cost)) {
      chosen_action = CHARSET_ACTION_COMPLEMENT;
      best_cost = estimated_complement_cost;
    }

    /* Fill out cep based on the chosen action */
    switch (chosen_action) {
      case CHARSET_ACTION_SCAN:
        /* Set up a scan */
        cep->cs = copy_charset(mm__, cs);
        charset_restrict_to_range(cep->cs, idx_start, idx_end);
        cep->idx_start = idx_start;
        cep->idx_end = idx_end;
        cep->split_point = 0;
        cep->action = CHARSET_ACTION_SCAN;
        cep->cost = estimated_scan_cost;
        cep->depth = depth;
        cep->children[0] = NULL;
        cep->children[1] = NULL;
        break;
      case CHARSET_ACTION_BITMAP:
        /* Set up a bitmap */
        cep->cs = copy_charset(mm__, cs);
        charset_restrict_to_range(cep->cs, idx_start, idx_end);
        cep->idx_start = idx_start;
        cep->idx_end = idx_end;
        cep->split_point = 0;
        cep->action = CHARSET_ACTION_BITMAP;
        cep->cost = estimated_bitmap_cost;
        cep->depth = depth;
        cep->children[0] = NULL;
        cep->children[1] = NULL;
        break;
      case CHARSET_ACTION_COMPLEMENT:
        /*
         * We have a CEP filled out we can just copy over; be sure to set
         * estimated_complement_cost = -1 so we know not to free it on the
         * way out.
         */
        memcpy(cep, &complement_cep, sizeof(complement_cep));
        memset(&complement_cep, 0, sizeof(complement_cep));
        estimated_complement_cost = -1;
        break;
      case CHARSET_ACTION_SPLIT:
        /*
         * We have a CEP filled out we can just copy over; be sure to set
         * estimated_split_cost = -1 so we know not to free it on the way
         * out.
         */
        memcpy(cep, &split_cep, sizeof(split_cep));
        memset(&split_cep, 0, sizeof(split_cep));
        estimated_split_cost = -1;
        break;
      default:
        /* Not supported */
        best_cost = -1;
        memset(cep, 0, sizeof(*cep));
        break;
    }
  }

  /* Free temporary CEPs if needed */

  if (estimated_complement_cost >= 0) {
    /*
     * We have a complement_cep we ended up not using; free its child and
     * charset
     */
    h_llvm_free_charset_exec_plan(mm__, complement_cep.children[0]);
    h_free(complement_cep.cs);
    memset(&complement_cep, 0, sizeof(complement_cep));
    estimated_complement_cost = -1;
  }

  if (estimated_split_cost >= 0) {
    /*
     * We have a split_cep we ended up not using; free its children and
     * charset.
     */
    h_llvm_free_charset_exec_plan(mm__, split_cep.children[0]);
    h_llvm_free_charset_exec_plan(mm__, split_cep.children[1]);
    h_free(split_cep.cs);
    memset(&split_cep, 0, sizeof(split_cep));
    estimated_split_cost = -1;
  }

  return best_cost;
}

/*
 * Build a charset exec plan for a charset
 */

static llvm_charset_exec_plan_t * h_llvm_build_charset_exec_plan(
    HAllocator* mm__, HCharset cs) {
  llvm_charset_exec_plan_t *cep = NULL;
  int best_cost;

  cep = h_new(llvm_charset_exec_plan_t, 1);
  best_cost = h_llvm_build_charset_exec_plan_impl(mm__, cs, NULL, cep, 1, NULL);

  if (best_cost < 0) {
    /* h_llvm_build_charset_exec_plan_impl() failed */
    h_free(cep);
    cep = NULL;
  }

  return cep;
}

/*
 * Consistency-check a charset exec plan
 */

static bool h_llvm_check_charset_exec_plan(llvm_charset_exec_plan_t *cep) {
  bool consistent = false;
  uint8_t i;

  if (cep) {
    /* Check that we have a charset */
    if (!(cep->cs)) goto done;
    /* Check that the range makes sense */
    if (cep->idx_start > cep->idx_end) goto done;
    /* Check that the charset is empty outside the range */
    for (i = 0; i < cep->idx_start; ++i) {
      /* Failed check */
      if (charset_isset(cep->cs, i)) goto done;
      /* Prevent wraparound */
      if (i == UINT8_MAX) break;
    }

    if (cep->idx_end < UINT8_MAX) {
      /* We break at the end */
      for (i = cep->idx_end + 1; ; ++i) {
        /* Failed check */
        if (charset_isset(cep->cs, i)) goto done;
        /* Prevent wraparound */
        if (i == UINT8_MAX) break;
      }
    }

    /* Minimum cost estimate is 0; complements and accepts can be free */
    if (cep->cost < 0) goto done;

    /* No split point unlesswe're CHARSET_ACTION_SPLIT */
    if (cep->action != CHARSET_ACTION_SPLIT && cep->split_point != 0) goto done;

    /* Action type dependent part */
    switch (cep->action) {
      case CHARSET_ACTION_ACCEPT:
      case CHARSET_ACTION_SCAN:
      case CHARSET_ACTION_BITMAP:
        /* These are always okay and have no children */
        if (cep->children[0] || cep->children[1]) goto done;
        consistent = true;
        break;
      case CHARSET_ACTION_COMPLEMENT:
        /* This has one child, which should have the same range */
        if (cep->children[1]) goto done;
        if (cep->children[0]) {
          if (cep->children[0]->idx_start == cep->idx_start &&
              cep->children[0]->idx_end == cep->idx_end) {
            /* The cost cannot be lower than the child */
            if (cep->cost < cep->children[0]->cost) goto done;
            /* Okay, we're consistent if the child node is */
            consistent = h_llvm_check_charset_exec_plan(cep->children[0]);
          }
        }
        break;
      case CHARSET_ACTION_SPLIT:
        /* This has two children, which should split the range */
        if (cep->children[0] && cep->children[1]) {
          if (cep->children[0]->idx_start == cep->idx_start &&
              cep->children[0]->idx_end + 1 == cep->children[1]->idx_start &&
              cep->children[1]->idx_end == cep->idx_end) {
            /* The split point must match the children */
            if (cep->split_point != cep->children[0]->idx_end) goto done;
            /*
             * The cost must be in the range defined by the children, + 1 for
             * the comparison at most
             */
            int child_min_cost = (cep->children[0]->cost < cep->children[1]->cost) ?
              cep->children[0]->cost : cep->children[1]->cost;
            int child_max_cost = (cep->children[0]->cost > cep->children[1]->cost) ?
              cep->children[0]->cost : cep->children[1]->cost;
            if ((cep->cost < child_min_cost) || (cep->cost > child_max_cost + 1)) goto done;
            /* Okay, we're consistent if both children are */
            consistent = h_llvm_check_charset_exec_plan(cep->children[0]) &&
                         h_llvm_check_charset_exec_plan(cep->children[1]);
          }
        }
        break;
      default:
        break;
    }
  }

 done:
  return consistent;
}

/*
 * Free a charset exec plan using the supplied allocator
 */

static void h_llvm_free_charset_exec_plan(HAllocator* mm__,
                                          llvm_charset_exec_plan_t *cep) {
  int n_children, i;

  if (cep) {
    n_children = 0;
    switch (cep->action) {
      case CHARSET_ACTION_COMPLEMENT:
        n_children = 1;
        break;
      case CHARSET_ACTION_SPLIT:
        n_children = 2;
        break;
      default:
        break;
    }

    for (i = 0; i < n_children; ++i) {
      h_llvm_free_charset_exec_plan(mm__, cep->children[i]);
    }
    h_free(cep->cs);
    h_free(cep);
  }
}

/*
 * Pretty-print a charset exec plan to stdout
 */

static void h_llvm_pretty_print_charset_exec_plan_impl(HAllocator *mm__, llvm_charset_exec_plan_t *cep,
                                                       const char *pfx_on_action_line, const char *pfx,
                                                       int depth) {
  const char *action_string = NULL, *pfx_incr = NULL;
  const char *pfx_incr_child_action = NULL, *pfx_incr_last_child = NULL;
  char *next_pfx = NULL, *next_pfx_child_action_line = NULL, *next_pfx_last_child = NULL;
  int n_children = 0, i, j, next_pfx_len;
  uint8_t ch;

  if (!cep) {
    action_string = "NULL";
  } else {
    switch (cep->action) {
      case CHARSET_ACTION_ACCEPT:
        action_string = "CHARSET_ACTION_ACCEPT";
        break;
      case CHARSET_ACTION_SCAN:
        action_string = "CHARSET_ACTION_SCAN";
        break;
      case CHARSET_ACTION_BITMAP:
        action_string = "CHARSET_ACTION_BITMAP";
        break;
      case CHARSET_ACTION_COMPLEMENT:
        action_string = "CHARSET_ACTION_COMPLEMENT";
        n_children = 1;
        break;
      case CHARSET_ACTION_SPLIT:
        action_string = "CHARSET_ACTION_SPLIT";
        n_children = 2;
        break;
      default:
        action_string = "UNKNOWN";
        break;
    }
  }

  if (n_children > 0) {
    pfx_incr = " | ";
  } else {
    pfx_incr = "   ";
  }


  if (depth > 0 || strlen(pfx_on_action_line) > 0) {
    printf("%s-%s\n", pfx_on_action_line, action_string);
    pfx_incr = (n_children > 0) ? " | " : "   ";
    pfx_incr_child_action = " +-";
    pfx_incr_last_child = "   ";
  } else {
    printf("%s\n", action_string);
    pfx_incr = (n_children > 0) ? "| " : "  ";
    pfx_incr_child_action = "+-";
    pfx_incr_last_child = "  ";
  }

  /*
   * Now do the charset, 8 lines of 32 bits with spaces in between to
   * fit [] range markers and | split point marker.
   */
  int open = 0, close = 0, split = 0;
  for (ch = 0, i = 0; i < 8; ++i) {
    /* Special case: [ should go before first char on line */
    if (ch == cep->idx_start) {
      printf("%s%s [", pfx, pfx_incr);
    } else {
      printf("%s%s  ", pfx, pfx_incr);
    }
    for (j = 0; j < 32; ++j, ++ch) {
      open = close = split = 0;
      /* Figure out markers, avoid wraparound */
      if (cep->idx_start != 0 && ch + 1 == cep->idx_start) {
        /* There should be a [ right after this char */
        open = 1;
      } else if (ch == cep->idx_end) {
        /* There should be a ] right after this char */
        close = 1;
      } else if (ch == cep->split_point &&
                 cep->action == CHARSET_ACTION_SPLIT) {
        /* There should be a | right after this char */
        split = 1;
      }

      if (charset_isset(cep->cs, ch)) printf("X");
      else printf(".");

      if (open) printf("[");
      else if (close) printf("]");
      else if (split) printf("|");
      else printf(" ");
    }
    printf("\n");
  }

  if (cep->action == CHARSET_ACTION_SPLIT) {
    printf("%s%s idx_start = %u, split_point = %u, idx_end = %u\n",
           pfx, pfx_incr, cep->idx_start, cep->split_point, cep->idx_end);
  } else {
    printf("%s%s idx_start = %u, idx_end = %u\n",
           pfx, pfx_incr, cep->idx_start, cep->idx_end);
  }

  printf("%s%s cost = %d, depth = %d\n", pfx, pfx_incr, cep->cost, cep->depth);

  if (n_children > 0) {
    if (n_children > 1) {
      next_pfx_len = strlen(pfx) + strlen(pfx_incr) + 1;
      next_pfx = h_new(char, next_pfx_len);
      snprintf(next_pfx, next_pfx_len, "%s%s", pfx, pfx_incr);
    } else {
      /* Won't be needed */
      next_pfx = NULL;
    }
    next_pfx_len = strlen(pfx) + strlen(pfx_incr_child_action) + 1;
    next_pfx_child_action_line = h_new(char, next_pfx_len);
    snprintf(next_pfx_child_action_line, next_pfx_len,
             "%s%s", pfx, pfx_incr_child_action);
    next_pfx_len = strlen(pfx) + strlen(pfx_incr_last_child) + 1;
    next_pfx_last_child = h_new(char, next_pfx_len);
    snprintf(next_pfx_last_child, next_pfx_len,
             "%s%s", pfx, pfx_incr_last_child);

    for (i = 0; i < n_children; ++i) {
      /* Space things out */
      printf("%s%s\n", pfx, pfx_incr);
      h_llvm_pretty_print_charset_exec_plan_impl(mm__, cep->children[i],
          next_pfx_child_action_line, (i + 1 == n_children) ? next_pfx_last_child : next_pfx,
          depth + 1);
    }

    if (next_pfx) h_free(next_pfx);
    h_free(next_pfx_last_child);
    h_free(next_pfx_child_action_line);
  }
}

static void h_llvm_pretty_print_charset_exec_plan(HAllocator *mm__, llvm_charset_exec_plan_t *cep) {
  /* Start at depth 0, and always emit an initial newline */
  printf("\n");
  h_llvm_pretty_print_charset_exec_plan_impl(mm__, cep, "", "", 0);
}

/* Forward declares for IR-emission functions */
static bool h_llvm_build_ir_for_scan(LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                     HCharset cs, uint8_t idx_start, uint8_t idx_end,
                                     LLVMValueRef r,
                                     LLVMBasicBlockRef in, LLVMBasicBlockRef yes, LLVMBasicBlockRef no);
static bool h_llvm_build_ir_for_split(HAllocator *mm__,
                                      LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                      llvm_charset_exec_plan_t *cep, LLVMValueRef r,
                                      LLVMBasicBlockRef in, LLVMBasicBlockRef yes, LLVMBasicBlockRef no);
static bool h_llvm_cep_to_ir(HAllocator* mm__,
                             LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                             LLVMValueRef r, llvm_charset_exec_plan_t *cep,
                             LLVMBasicBlockRef in, LLVMBasicBlockRef yes, LLVMBasicBlockRef no);

/*
 * Build IR for a CHARSET_ACTION_SCAN
 */

static bool h_llvm_build_ir_for_scan(LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                     HCharset cs, uint8_t idx_start, uint8_t idx_end,
                                     LLVMValueRef r,
                                     LLVMBasicBlockRef in, LLVMBasicBlockRef yes, LLVMBasicBlockRef no) {
  if (!cs) return false;
  if (idx_start > idx_end) return false;

  /*
   * Scan the range of indices, and for each thing in the charset,
   * compare and conditional branch.
   */
  LLVMPositionBuilderAtEnd(builder, in);

  for (int i = idx_start; i <= idx_end; ++i) {
    if (charset_isset(cs, i)) {
      char bbname[16];
      uint8_t c = (uint8_t)i;
      snprintf(bbname, 16, "cs_memb_%02x", c);
      LLVMValueRef icmp = LLVMBuildICmp(builder, LLVMIntEQ,
          LLVMConstInt(LLVMInt8Type(), c, 0), r, "c == r");
      LLVMBasicBlockRef bb = LLVMAppendBasicBlock(func, bbname);
      LLVMBuildCondBr(builder, icmp, yes, bb);
      LLVMPositionBuilderAtEnd(builder, bb);
    }
  }

  LLVMBuildBr(builder, no);

  return true;
}

/*
 * Build IR for a CHARSET_ACTION_SPLIT
 */

static bool h_llvm_build_ir_for_split(HAllocator *mm__,
                                      LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                      llvm_charset_exec_plan_t *cep, LLVMValueRef r,
                                      LLVMBasicBlockRef in, LLVMBasicBlockRef yes, LLVMBasicBlockRef no) {
  char name[18];
  bool left_ok, right_ok;

  /* Split validation */
  if (!cep) return false;
  if (cep->action != CHARSET_ACTION_SPLIT) return false;
  if (cep->idx_start >= cep->idx_end) return false;
  if (cep->split_point < cep->idx_start) return false;
  if (cep->split_point >= cep->idx_end) return false;
  if (!(cep->children[0] && cep->children[1])) return false;
  if (cep->idx_start != cep->children[0]->idx_start) return false;
  if (cep->split_point != cep->children[0]->idx_end) return false;
  if (cep->split_point + 1 != cep->children[1]->idx_start) return false;
  if (cep->idx_end != cep->children[1]->idx_end) return false;

  /*
   * Compare the value against the split point, and branch to the left
   * child if <=, right child if >.
   */
  snprintf(name, 18, "cs_split_left_%02X", cep->split_point);
  LLVMBasicBlockRef left = LLVMAppendBasicBlock(func, name);
  snprintf(name, 18, "cs_split_right_%02X", cep->split_point);
  LLVMBasicBlockRef right = LLVMAppendBasicBlock(func, name);
  LLVMPositionBuilderAtEnd(builder, in);
  snprintf(name, 18, "r <= %02X", cep->split_point);
  LLVMValueRef icmp = LLVMBuildICmp(builder, LLVMIntULE,
      r, LLVMConstInt(LLVMInt8Type(), cep->split_point, 0), name);
  LLVMBuildCondBr(builder, icmp, left, right);

  /*
   * Now build the subtrees starting from each of the output basic blocks
   * of the comparison.
   */
  left_ok = h_llvm_cep_to_ir(mm__, mod, func, builder, r, cep->children[0], left, yes, no);
  right_ok = h_llvm_cep_to_ir(mm__, mod, func, builder, r, cep->children[1], right, yes, no);

  return left_ok && right_ok;
}

/*
 * Turn an llvm_charset_exec_plan_t into IR
 */

static bool h_llvm_cep_to_ir(HAllocator* mm__,
                             LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                             LLVMValueRef r, llvm_charset_exec_plan_t *cep,
                             LLVMBasicBlockRef in, LLVMBasicBlockRef yes, LLVMBasicBlockRef no) {
  bool rv;

  if (!cep) return false;

  switch (cep->action) {
    case CHARSET_ACTION_SCAN:
      rv = h_llvm_build_ir_for_scan(mod, func, builder,
          cep->cs, cep->idx_start, cep->idx_end, r, in, yes, no);
      break;
    case CHARSET_ACTION_ACCEPT:
      /* Easy case; just unconditionally branch to the yes output */
      LLVMPositionBuilderAtEnd(builder, in);
      LLVMBuildBr(builder, yes);
      rv = true;
      break;
    case CHARSET_ACTION_BITMAP:
#ifdef HAMMER_LLVM_CHARSET_DEBUG
      fprintf(stderr,
              "CHARSET_ACTION_BITMAP not yet implemented (cep %p)\n",
              (void *)cep);
#endif /* defined(HAMMER_LLVM_CHARSET_DEBUG) */
      rv = false;
      break;
    case CHARSET_ACTION_COMPLEMENT:
      /* This is trivial; just swap the 'yes' and 'no' outputs and build the child */
      rv = h_llvm_cep_to_ir(mm__, mod, func, builder, r, cep->children[0], in, no, yes);
      break;
    case CHARSET_ACTION_SPLIT:
      rv = h_llvm_build_ir_for_split(mm__, mod, func, builder, cep, r, in, yes, no);
      break;
    default:
      /* Unknown action type */
#ifdef HAMMER_LLVM_CHARSET_DEBUG
      fprintf(stderr,
              "cep %p has unknown action type\n",
              (void *)cep);
#endif /* defined(HAMMER_LLVM_CHARSET_DEBUG) */
      rv = false;
      break;
  }

  return rv;
}

/*
 * Construct LLVM IR to decide if a runtime value is a member of a compile-time
 * character set, and branch depending on the result.
 *
 * Parameters:
 *  - mod [in]: an LLVMModuleRef
 *  - func [in]: an LLVMValueRef to the function to add the new basic blocks
 *  - builder [in]: an LLVMBuilderRef, positioned appropriately
 *  - r [in]: an LLVMValueRef to the value to test
 *  - cs [in]: the HCharset to test membership in
 *  - yes [in]: the basic block to branch to if r is in cs
 *  - no [in]: the basic block to branch to if r is not in cs
 *
 * Returns: true on success, false on failure
 */

bool h_llvm_make_charset_membership_test(HAllocator* mm__,
                                         LLVMModuleRef mod, LLVMValueRef func, LLVMBuilderRef builder,
                                         LLVMValueRef r, HCharset cs,
                                         LLVMBasicBlockRef yes, LLVMBasicBlockRef no) {
  /*
   * A charset is a 256-element bit array, 32 bytes long in total.  Ours is
   * static at compile time, so we can try to construct minimal LLVM IR for
   * this particular charset.  In particular, we should handle cases like
   * only one or two bits being set, or a long consecutive range, efficiently.
   *
   * In LLVM IR, we can test propositions like r == x, r <= x, r >= x and their
   * negations efficiently, so the challenge here is to turn a character map
   * into a minimal set of such propositions.
   *
   * We achieve this by building a tree of actions to minimize a cost metric,
   * and then transforming the tree into IR.
   */

  bool rv;

  /* Try building a charset exec plan */
  llvm_charset_exec_plan_t *cep = h_llvm_build_charset_exec_plan(mm__, cs);
  if (!cep) {
    fprintf(stderr, "got null from h_llvm_build_charset_exec_plan()\n");
    return false;
  }

#ifdef HAMMER_LLVM_CHARSET_DEBUG
  bool ok = h_llvm_check_charset_exec_plan(cep);
  if (ok) fprintf(stderr, "cep %p passes consistency check\n", (void *)cep);
  else fprintf(stderr, "cep %p fails consistency check\n", (void *)cep);
  h_llvm_pretty_print_charset_exec_plan(mm__, cep);
  if (!ok) {
    fprintf(stderr, "h_llvm_make_charset_membership_test() error-exiting "
            "because consistency check failed\n");
    h_llvm_free_charset_exec_plan(mm__, cep);
    cep = NULL;
    return false;
  }
#endif /* defined(HAMMER_LLVM_CHARSET_DEBUG) */

  /*
   * XXX Note on memoization:
   *
   * How common is it for this to occur multiple times in a parser with the
   * same charset?  If so, we will end up emitting code which differs only in
   * its yes and no output basic blocks each time.  Does LLVM IR have an
   * equivalent of MIPS jr?  Is there a significant performance penalty vs.
   * LLVMBuildBr()?  If yes and no respectively, we should consider memoizing
   * by charset using it and building a wrapper around it that just varies
   * the output blocks to reduce emitted code size.
   */

  /* Create input block */
  LLVMBasicBlockRef start = LLVMAppendBasicBlock(func, "cs_start");
  /*
   * Make unconditional branch into input block from wherever our caller
   * had us positioned.
   */
  LLVMBuildBr(builder, start);

  rv = h_llvm_cep_to_ir(mm__, mod, func, builder, r, cep, start, yes, no);

  h_llvm_free_charset_exec_plan(mm__, cep);
  cep = NULL;

  return rv;
}

#endif /* defined(HAMMER_LLVM_BACKEND) */
