#include <stdio.h>

#include "debug.h"

void print_bytecode(bytecode* code) {
#define print(s) (printf("%4d: %*s%s\n", i, indent, "", s))
#define print_arg(s) (printf("%4d: %*s%s %d\n", i, indent, "", s, c.arg))
  int i = 0;
  int indent = 0;
  while(1) {
    bytecode c = code[i++];
    switch (c.code) {
      case INC: print_arg("inc"); break;
      case DEC: print_arg("dec"); break;
      case FWD: print_arg("fwd"); break;
      case BCK: print_arg("bck"); break;
      case PRN: print("prn"); break;
      case ZERO: print("zero"); break;
      case READ: print("read"); break;
      case STARTL: print_arg("start loop"); indent += 2; break;
      case ENDL: indent -= 2; print_arg("end loop"); break;

      case SEND: print_arg("send"); break;
      case RECV: print("receive"); break;

      case MOVE_PTR: print_arg("move ptr"); break;
      case MOVE_DATA: print_arg("move data"); break;

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
