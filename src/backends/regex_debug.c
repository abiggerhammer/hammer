// Intended to be included from regex_debug.c
#define _GNU_SOURCE
#include <stdio.h>
#include <malloc.h>



// This is some spectacularly non-portable code... but whee!
#include <dlfcn.h>
char* getsym(void* addr) {
  Dl_info dli;
  char* retstr;
  if (dladdr(addr, &dli) != 0 && dli.dli_sname != NULL) {
    if (dli.dli_saddr == addr)
      return strdup(dli.dli_sname);
    else
      asprintf(&retstr, "%s+0x%lx", dli.dli_sname, addr - dli.dli_saddr);
  } else
    asprintf(&retstr, "%p", addr);

  return retstr;
}

const char* rvm_op_names[RVM_OPCOUNT] = {
  "ACCEPT",
  "GOTO",
  "FORK",
  "PUSH",
  "ACTION",
  "CAPTURE",
  "EOF",
  "MATCH",
  "STEP"
};

const char* svm_op_names[SVM_OPCOUNT] = {
  "PUSH",
  "NOP",
  "ACTION",
  "CAPTURE",
  "ACCEPT"
};

void dump_rvm_prog(HRVMProg *prog) {
  char* symref;
  for (unsigned int i = 0; i < prog->length; i++) {
    HRVMInsn *insn = &prog->insns[i];
    printf("%4d %-10s", i, rvm_op_names[insn->op]);
    switch (insn->op) {
    case RVM_GOTO:
    case RVM_FORK:
      printf("%hd\n", insn->arg);
      break;
    case RVM_ACTION:
      symref = getsym(prog->actions[insn->arg].action);
      // TODO: somehow format the argument to action
      printf("%s\n", symref);
      free(symref);
      break;
    case RVM_MATCH: {
      uint8_t low, high;
      low = insn->arg & 0xff;
      high = (insn->arg >> 8) & 0xff;
      if (high > low)
	printf("NONE\n");
      else {
	if (low >= 0x32 && low <= 0x7e)
	  printf("%02hhx ('%c')", low, low);
	else
	  printf("%02hhx", low);
	
	if (high >= 0x32 && high <= 0x7e)
	  printf(" - %02hhx ('%c')\n", high, high);
	else
	  printf(" - %02hhx\n", high);
      }
      break;
    }
    default:
      printf("\n");
    }
  }
}
