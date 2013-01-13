#include "../internal.h"
#include "../parsers/parser_internal.h"

#undef a_new
#define a_new(typ, count) a_new_(arena, typ, count);
// Stack VM
typedef enum HSVMOp_ {
  SVM_PUSH, // Push a mark. There is no VM insn to push an object.
  SVM_NOP, // Used to start the chain, and possibly elsewhere. Does nothing.
  SVM_ACTION, // Same meaning as RVM_ACTION
  SVM_CAPTURE, // Same meaning as RVM_CAPTURE
  SVM_ACCEPT,
} HSVMOp;

typedef struct HRVMTrace_ {
  struct HRVMTrace_ *next; // When parsing, these are
			   // reverse-threaded. There is a postproc
			   // step that inverts all the pointers.
  uint16_t arg;
  uint8_t opcode;
} HRVMTrace;

typedef struct HRVMThread_ {
  HRVMTrace *trace;
  uint16_t ip;
} HRVMThread;
    
void* h_rvm_run__m(HAllocator *mm__, HRVMProg *prog, const char* input, size_t len) {
  HArena *arena = h_new_arena(mm__, 0);
  HRVMTrace **heads_p = a_new(HRVMTrace*, prog->length),
    **heads_n = a_new(HRVMTrace*, prog->length), **heads_t;

  
  uint8_t *insn_seen = a_new(uint8_t, prog->length); // 0 -> not seen, 1->processed, 2->queued
  HRVMThread *ip_queue = a_new(HRVMThread, prog->length);
  size_t ipq_top;

#define THREAD ip_queue[ipq_top-1]
#define PUSH_SVM(op_, arg_) do { \
	  HRVMTrace *nt = a_new(HRVMTrace, 1); \
	  nt->arg = (arg_);		       \
	  nt->opcode = (op_);		       \
	  nt->next = THREAD.trace;	       \
	  THREAD.trace = nt;		       \
  } while(0)
    
  heads_n[0] = a_new(HRVMTrace, 1); // zeroing
  heads_n[0]->opcode = SVM_NOP;

  size_t off = 0;
  int live_threads = 1;
  for (off = 0; off <= len; off++) {
    uint8_t ch = ((off == len) ? 0 : input[off]);
    size_t ip_s, ip;
    /* scope */ {
      HRVMTrace **heads_t;
      heads_t = heads_n;
      heads_n = heads_p;
      heads_p = heads_t;
      memset(heads_n, 0, prog->length * sizeof(*heads_n));
    }
    memset(insn_seen, 0, prog->length); // no insns seen yet
    if (!live_threads)
      return NULL;
    live_threads = 0;
    for (ip_s = 0; ip_s < prog->length; ip_s++) {
      ipq_top = 1;
      // TODO: Write this as a threaded VM
      if (!heads_p[ip_s])
	continue;
      THREAD.ip = ip_s;

      uint8_t hi, lo;
      uint16_t arg;
      while(ipq_top > 0) {
	if (insns_seen[THREAD.ip] == 1)
	  continue;
	insns_seen[THREAD.ip] = 1;
	arg = prog->insns[THREAD.ip].arg;
	switch(prog->insns[THREAD.ip].op) {
	case RVM_ACCEPT:
	  // TODO: save current SVM pos, and jump to end
	  abort();
	case RVM_MATCH:
	  // Doesn't actually validate the "must be followed by MATCH
	  // or STEP. It should. Preproc perhaps?
	  hi = (arg >> 8) & 0xff;
	  lo = arg & 0xff;
	  THREAD.ip++;
	  if (ch < lo && ch > hi)
	    ipq_top--; // terminate thread
	  goto next_insn;
	case RVM_GOTO:
	  THREAD.ip = arg;
	  goto next_insn;
	case RVM_FORK:
	  THREAD.ip++;
	  if (!insns_seen[arg]) {
	    insns_seen[THREAD.ip] = 2;
	    HRVMTrace* tr = THREAD.trace;
	    ipq_top++;
	    THREAD.ip = arg;
	    THREAD.trace = tr;
	  }
	  goto next_insn;
	case RVM_PUSH:
	  PUSH_SVM(SVM_PUSH, off);
	  THREAD.ip++;
	  goto next_insn;
	case RVM_ACTION:
	  PUSH_SVM(SVM_ACTION, arg);
	  THREAD.ip++;
	  goto next_insn;
	case RVM_CAPTURE:
	  PUSH_SVM(SVM_CAPTURE, 0);
	  THREAD.ip++;
	  goto next_insn;
	case RVM_EOF:
	  THREAD.ip++;
	  if (off != len)
	    ipq_top--; // Terminate thread
	  goto next_insn;
	case RVM_STEP:
	  // save thread
	  live_threads++;
	  heads_n[THREAD.ip++] = THREAD.trace;
	  ipq_top--;
	  goto next_insn;
	}
      next_insn:
	
      }
      
      
      
      
  
}
