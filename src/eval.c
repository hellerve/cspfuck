#include "stdio.h"

#include "eval.h"

void eval(bytecode* code) {
  int i = 0;
  int h = 0;
  int t[30000];

  for (int idx = 0; idx < 30000; idx++) t[idx] = 0;

  while(1) {
    bytecode c = code[i++];
    switch (c.code) {
      case INC: t[h]++; break;
      case DEC: t[h]--; break;
      case FWD: h++; break;
      case BCK: h--; break;
      case PRN: printf("%c", t[h]); break;
      case READ: scanf("%c", (char*)&t[h]); break;
      case STARTL: if(!t[h]) i = c.arg; break;
      case ENDL: if(t[h]) i = c.arg; break;

      //case SEND: print_arg("send", c.arg, indent); break;
      //case RECV: print("receive", indent); break;

      case HALT: return;
    }
  }
}

void eval_actors(actors* ac) {
  int i;
  for (i = 0; i < ac->num; i++) eval(ac->code[i]);
}
