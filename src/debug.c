#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
  #define print_times(s) { for (j = 0; j < c.arg; j++) printf(s); }
  int i = 0;
  int j;
  while(1) {
    bytecode c = code[i++];
    switch (c.code) {
      case INC: print_times("+"); break;
      case DEC: print_times("-"); break;
      case FWD: print_times(">"); break;
      case BCK: print_times("<"); break;
      case PRN: printf("."); break;
      case ZERO: printf("0"); break;
      case MOVE_PTR:
        printf("[");
        for (j = 0; j < abs(c.arg); j++) printf(c.arg > 0 ? ">" : "<");
        printf("]");
        break;
      case MOVE_DATA:
        printf("[-");
        for (int j = 0; j < abs(c.arg); j++) printf(c.arg > 0 ? ">" : "<");
        printf("+");
        for (int j = 0; j < abs(c.arg); j++) printf(c.arg > 0 ? "<" : ">");
        printf("]");
        break;
      case READ: printf(","); break;
      case STARTL: printf("["); break;
      case ENDL: printf("]"); break;

      case SEND: if(c.arg) printf("^"); else printf("v"); break;
      case RECV: printf("u"); break;

      case HALT: return;
    }
  }
  #undef print_times
}

void print_src(actors* ac) {
  for (int i = 0; i < ac->num; i++) {
    print_bytecode_src(ac->code[i]);
    printf("\n\n");
  }
}

uint32_t calc_offset(uint32_t jump_frm, uint32_t jump_to) {
  if (jump_to >= jump_frm) return jump_to - jump_frm;
  else return ~((uint32_t)jump_frm-jump_to) + 1;
}

void print_faux_assembly_bytecode(long offset, bytecode* code) {
#define print(s) (printf("0x%lx: %s\n", idx+offset, s))
#define print_arg(s) (printf("0x%lx: %s   $0x%x, (%%r13)\n", idx+offset, s, c.arg))
#define print_arg_ref(s) (printf("0x%lx: %s   $0x%x, %%r13b\n", idx+offset, s, c.arg))
#define print_syscall(d) {\
  printf("0x%lx: movq   0x200000%d, %%rax\n", idx+offset, d);\
  idx+=7;\
  print("movq   0x1, %%rdi");\
  idx+=7;\
  print("movq   %%r13, %%rsi");\
  idx+=3;\
  print("movq   0x1, %%rdx");\
  idx+=7;\
  print("syscall ");\
  idx+=2;\
}
  int i = 0;
  long idx = 10;
  size_t loop_stack[256];
  int loop_depth = 0;
  printf("0x%lx: movabsq $random-addr, %%r13\n", offset);
  while(1) {
    bytecode c = code[i++];
    switch (c.code) {
      case INC: print_arg("addb"); idx+=5;break;
      case DEC: print_arg("subb"); idx+=5;break;
      case FWD: print_arg_ref("addb"); idx+=4;break;
      case BCK: print_arg_ref("subb"); idx+=4;break;
      case PRN: print_syscall(4); break;
      case ZERO: print("movb   $0x0, (%r13)"); idx+=5;break;
      case READ: print_syscall(3); break;
      case STARTL:
        print("cmpb   $0x0, (%r13)");
        idx+=5;
        printf("0x%lx: je     0x%lx\n", idx+offset, c.arg+offset);
        loop_stack[loop_depth++] = idx;
        idx+=6;
        break;
      case ENDL: {
        size_t begin = loop_stack[--loop_depth];

        print("cmpb   $0x0, (%r13)");
        idx+=5;

        size_t jmp_frm = idx + 6;
        size_t jmp_to = begin + 6;
        uint32_t pcrel_offset = calc_offset(jmp_frm, jmp_to);
        printf("0x%lx: jne    0x%x\n", idx+offset, pcrel_offset);
        idx+=6;
        break;
      }
      case HALT:
        print("retq");
        return;
    }
  }
#undef print
#undef print_arg
#undef print_arg_ref
#undef print_syscall
}

void print_faux_assembly(long offset, actors* ac) {
  for (int i = 0; i < ac->num; i++) {
    print_faux_assembly_bytecode(offset, ac->code[i]);
  }
}
