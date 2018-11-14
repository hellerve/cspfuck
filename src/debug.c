#include <stdio.h>

#include "debug.h"

void print_bytecode(bytecode* code) {
#define print(i, s, ind) (printf("%4d: %*s%s\n", i, ind, "", s))
#define print_arg(i, s, a, ind) (printf("%4d: %*s%s %d\n", i, ind, "", s, a))
  int i = 0;
  int indent = 0;
  while(1) {
    bytecode c = code[i++];
    switch (c.code) {
      case INC: print(i, "inc", indent); break;
      case DEC: print(i, "dec", indent); break;
      case FWD: print(i, "fwd", indent); break;
      case BCK: print(i, "bck", indent); break;
      case PRN: print(i, "prn", indent); break;
      case ZERO: print(i, "zero", indent); break;
      case READ: print(i, "read", indent); break;
      case STARTL: print_arg(i, "start loop", c.arg, indent); indent += 2; break;
      case ENDL: indent -= 2; print_arg(i, "end loop", c.arg, indent); break;

      case SEND: print_arg(i, "send", c.arg, indent); break;
      case RECV: print(i, "receive", indent); break;

      case HALT: return;
    }
  }
#undef print
#undef print_arg
}

void print_actors(actors* ac) {
  for (int i = 0; i < ac->num; i++) {
    printf("Bytecode for Actor %d:\n=====================\n", i);
    print_bytecode(ac->code[i]);
    printf("=====================\n");
  }
}

void print_bytecode_src(bytecode* code) {
  int i = 0;
  while(1) {
    bytecode c = code[i++];
    switch (c.code) {
      case INC: printf("+"); break;
      case DEC: printf("-"); break;
      case FWD: printf(">"); break;
      case BCK: printf("<"); break;
      case PRN: printf("."); break;
      case ZERO: printf("0"); break;
      case READ: printf(","); break;
      case STARTL: printf("["); break;
      case ENDL: printf("]"); break;

      case SEND: if(c.arg) printf("^"); else printf("v"); break;
      case RECV: printf("u"); break;

      case HALT: return;
    }
  }
}

void print_src(actors* ac) {
  for (int i = 0; i < ac->num; i++) {
    print_bytecode_src(ac->code[i]);
    printf("\n\n");
  }
}
