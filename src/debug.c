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
