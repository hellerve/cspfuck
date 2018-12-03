#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "eval.h"

#define TAPE_LEN 30000

typedef struct {
  int num;
  int* up;
  int* up_written;
  int* down;
  int* down_written;
  bytecode* code;
} actor_ctx;

unsigned int relative_32bit_offset(int jump_frm, int jump_to) {
  if (jump_to >= jump_frm) return jump_to - jump_frm;
  else return ~((unsigned int)jump_frm-jump_to) + 1;
}

typedef void (*jit_fn)(void);

void* jit(void* arg) {
#define add_at(x, i) {\
  sys[i] = x;\
}
#define add(x) {\
  sys = realloc(sys, ++syslen);\
  sys[syslen-1] = x;\
}
#define add32_at(x, i) {\
  add_at((x) & 0xff, i);\
  add_at(((x) >> 8) & 0xff, i+1);\
  add_at(((x) >> 16) & 0xff, i+2);\
  add_at(((x) >> 24) & 0xff, i+3);\
}
#define add32(x) {\
  add(x & 0xff);\
  add(((x) >> 8) & 0xff);\
  add(((x) >> 16) & 0xff);\
  add(((x) >> 24) & 0xff);\
}
#define add64(x) {\
  add32((x) & 0xffffffff);\
  add32((x >> 32) & 0xffffffff);\
}
  int i = 0;
  unsigned short t[TAPE_LEN];
  actor_ctx* ctx = (actor_ctx*) arg;
  bytecode c;
  unsigned int syslen = 0;
  unsigned char* sys = NULL;
  int loop_stack[256];
  int loop_depth = 0;

  for (int idx = 0; idx < TAPE_LEN; idx++) t[idx] = 0;

  add(0x49);
  add(0xbd);
  add64((uint64_t)&t);

  while (1) {
    c = ctx->code[i++];
    switch (c.code) {
      case INC: add(0x41); add(0x80); add(0x45); add(0x00); add(c.arg); break;
      case DEC: add(0x41); add(0x80); add(0x6d); add(0x00); add(c.arg); break;
      case FWD: add(0x41); add(0x80); add(0xC5); add(c.arg); break;
      case BCK: add(0x41); add(0x80); add(0xCD); add(c.arg); break;
      case PRN:
        add(0x48); add(0xc7); add(0xc0); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x48); add(0xc7); add(0xc7); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x4c); add(0x89); add(0xee);
        add(0x48); add(0xc7); add(0xc2); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x0f); add(0x05);
        break;
      case READ:
        add(0x48); add(0xC7); add(0xC0); add(0x00); add(0x00); add(0x00); add(0x00);
        add(0x48); add(0xC7); add(0xC7); add(0x00); add(0x00); add(0x00); add(0x00);
        add(0x4C); add(0x89); add(0xEE);
        add(0x48); add(0xC7); add(0xC2); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x0F); add(0x05);
        break;
      case STARTL:
        add(0x41); add(0x80); add(0x7d); add(0x00); add(0x00);

        loop_stack[loop_depth++] = syslen;
        add(0x0F); add(0x84); add32(0);
        break;
      case ENDL: {
        int begin = loop_stack[--loop_depth];
        add(0x41); add(0x80); add(0x7d); add(0x00); add(0x00);

        int jmp_frm = syslen + 6;
        int jmp_to = begin + 6;
        int pcrel_offset = relative_32bit_offset(jmp_frm, jmp_to);

        add(0x0F); add(0x85);
        add32(pcrel_offset);

        jmp_frm = begin + 6;
        jmp_to = syslen;
        pcrel_offset = relative_32bit_offset(jmp_frm, jmp_to);
        add32_at(pcrel_offset, begin+2);
        break;
      }
      case HALT:
      default:
        goto jit_start;
    }
  }
jit_start:
  add(0xc3);

  void* mem = mmap(0, syslen, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  memcpy(mem, sys, syslen);
  mprotect(mem, syslen, PROT_READ | PROT_EXEC);

  jit_fn f = (jit_fn)mem;

  f();

  free(sys);

  return NULL;
#undef add
#undef add32
#undef add64
#undef add32_at
}

void eval_actors(actors* ac) {
  int i;
  pthread_t ts[ac->num];
  actor_ctx ctxs[ac->num];
  int* down = NULL;
  int* down_written = NULL;
  for (i = 0; i < ac->num; i++) {
    actor_ctx ctx;
    ctx.num = i;
    ctx.down = down;
    ctx.down_written = down_written;
    if (i < ac->num-1) {
      ctx.up = malloc(sizeof(int));
      ctx.up_written = malloc(sizeof(int));
      *ctx.up_written = 0;
    } else {
      ctx.up = NULL;
      ctx.up_written = NULL;
    }
    ctx.code = ac->code[i];
    ctxs[i] = ctx;
    if(ac->num == 1) {
      // if we just have one thread, we eval it directly; buys us about 2% perf
      jit(&ctx);
      free(ctx.up);
      free(ctx.up_written);
      return;
    } else {
      pthread_create(&ts[i], NULL, jit, &ctxs[i]);
    }
    down = ctx.up;
    down_written = ctx.up_written;
  }

  for (i = 0; i < ac->num; i++) {
    pthread_join(ts[i], NULL);
    actor_ctx ctx = ctxs[i];
    if (ctx.up) { free(ctx.up); free(ctx.up_written); }
  }
}
