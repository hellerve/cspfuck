#define _DEFAULT_SOURCE
#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "eval.h"

#define TAPE_LEN 30000

typedef struct {
  int num;
  int* up_in;
  int* up_written_in;
  int* up_out;
  int* up_written_out;
  int* down_in;
  int* down_written_in;
  int* down_out;
  int* down_written_out;
  bytecode* code;
} actor_ctx;

void recv(actor_ctx* ctx, int* cl) {
  while ((!ctx->up_written_in || !(*ctx->up_written_in)) &&
         (!ctx->down_written_in || !(*ctx->down_written_in))) usleep(100);
  if (ctx->up_written_in && *ctx->up_written_in) {
    *cl = *ctx->up_in;
    *ctx->up_written_in = 0;
  } else if (ctx->down_written_in && *ctx->down_written_in) {
    *cl = *ctx->down_in;
    *ctx->down_written_in = 0;
  }
}

void send_down(actor_ctx* ctx, int* cl) {
  if (!ctx->down_out) {
    fprintf(stderr,
      "Actor %d tried to write to non-existant down channel, ignoring.",
      ctx->num
    );
    return;
  }
  *ctx->down_out = *cl;
  *ctx->down_written_out = 1;
}

void send_up(actor_ctx* ctx, int* cl) {
  if (!ctx->up_out) {
    fprintf(stderr,
      "Actor %d tried to write to non-existant up channel, ignoring.",
      ctx->num
    );
    return;
  }
  *ctx->up_out = *cl;
  *ctx->up_written_out = 1;
}

uint32_t relative_32bit_offset(uint32_t jump_frm, uint32_t jump_to) {
  if (jump_to >= jump_frm) {
    size_t diff = jump_to - jump_frm;
    assert(diff < (1ull << 31));
    return diff;
  } else {
    size_t diff = jump_frm - jump_to;
    assert(diff-1 < (1ull << 31));
    uint32_t diff_unsigned = (uint32_t)diff;
    return ~diff_unsigned + 1;
  }
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
  add((x) & 0xff);\
  add(((x) >> 8) & 0xff);\
  add(((x) >> 16) & 0xff);\
  add(((x) >> 24) & 0xff);\
}
#define add64(x) {\
  add32((x) & 0xffffffff);\
  add32(((x) >> 32) & 0xffffffff);\
}
  int i = 0;
  uint8_t t[TAPE_LEN];
  actor_ctx* ctx = (actor_ctx*) arg;
  bytecode c;
  size_t syslen = 0;
  uint8_t* sys = NULL;
  size_t loop_stack[2048];
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
      case ZERO: add(0x41); add(0xc6); add(0x45); add(0x00); add(0x00); break;
      case FWD: add(0x49); add(0x83); add(0xc5); add(c.arg); break;
      case BCK: add(0x49); add(0x83); add(0xed); add(c.arg); break;
      case PRN:
#ifdef __APPLE__
        add(0x48); add(0xc7); add(0xc0); add(0x04); add(0x00); add(0x00); add(0x02);
#else
        add(0x48); add(0xc7); add(0xc0); add(0x01); add(0x00); add(0x00); add(0x00);
#endif
        add(0x48); add(0xc7); add(0xc7); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x4c); add(0x89); add(0xee);
        add(0x48); add(0xc7); add(0xc2); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x0f); add(0x05);
        break;
      case READ:
#ifdef __APPLE__
        add(0x48); add(0xc7); add(0xc0); add(0x03); add(0x00); add(0x00); add(0x02);
#else
        add(0x48); add(0xc7); add(0xc0); add(0x00); add(0x00); add(0x00); add(0x00);
#endif
        add(0x48); add(0xc7); add(0xc7); add(0x00); add(0x00); add(0x00); add(0x00);
        add(0x4c); add(0x89); add(0xee);
        add(0x48); add(0xc7); add(0xc2); add(0x01); add(0x00); add(0x00); add(0x00);
        add(0x0f); add(0x05);
        break;
      case STARTL:
        add(0x41); add(0x80); add(0x7d); add(0x00); add(0x00);

        loop_stack[loop_depth++] = syslen;
        add(0x0f); add(0x84); add32(0);
        break;
      case ENDL: {
        size_t begin = loop_stack[--loop_depth];
        add(0x41); add(0x80); add(0x7d); add(0x00); add(0x00);

        size_t jmp_frm = syslen + 6;
        size_t jmp_to = begin + 6;
        uint32_t pcrel_offset = relative_32bit_offset(jmp_frm, jmp_to);

        add(0x0f); add(0x85);
        add32(pcrel_offset);

        jmp_frm = begin + 6;
        jmp_to = syslen;
        pcrel_offset = relative_32bit_offset(jmp_frm, jmp_to);
        add32_at(pcrel_offset, begin+2);
        break;
      }
      case RECV: {
        add(0x55);
        add(0x4c); add(0x89); add(0xee);
        add(0x48); add(0xbf); add64((uint64_t)ctx);
        add(0x49); add(0xbe); add64((uint64_t)recv);
        add(0x41); add(0xff); add(0xd6);
        add(0x5d);
        break;
      }
      case SEND: {
        add(0x55);
        add(0x48); add(0xbf); add64((uint64_t)ctx);
        add(0x49); add(0xbe); add64((uint64_t)(c.arg ? send_up : send_down));
        add(0x41); add(0xff); add(0xd6);
        add(0x5d);
        break;
      }
      case HALT:
        goto jit_start;
      default:
        break;
    }
  }
jit_start:
  add(0xc3);

  void* mem = mmap(0, syslen, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  memcpy(mem, sys, syslen);
  mprotect(mem, syslen, PROT_READ | PROT_EXEC);

  jit_fn f = (jit_fn)mem;

  f();

  munmap(mem, syslen);
  //free(sys);

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
  int* down_in = NULL;
  int* down_written_in = NULL;
  int* down_out = NULL;
  int* down_written_out = NULL;
  for (i = 0; i < ac->num; i++) {
    actor_ctx ctx;
    ctx.num = i;
    ctx.down_in = down_in;
    ctx.down_written_in = down_written_in;
    ctx.down_out = down_out;
    ctx.down_written_out = down_written_out;
    if (i < ac->num-1) {
      ctx.up_in = malloc(sizeof(int));
      ctx.up_written_in = malloc(sizeof(int));
      *ctx.up_written_in = 0;
      ctx.up_out = malloc(sizeof(int));
      ctx.up_written_out = malloc(sizeof(int));
      *ctx.up_written_out = 0;
    } else {
      ctx.up_in = NULL;
      ctx.up_written_in = NULL;
      ctx.up_out = NULL;
      ctx.up_written_out = NULL;
    }
    ctx.code = ac->code[i];
    ctxs[i] = ctx;
    if(ac->num == 1) {
      // if we just have one thread, we eval it directly; buys us about 2% perf
      jit(&ctx);
      free(ctx.up_in);
      free(ctx.up_written_in);
      free(ctx.up_out);
      free(ctx.up_written_out);
      return;
    } else {
      pthread_create(&ts[i], NULL, jit, &ctxs[i]);
    }
    down_in = ctx.up_out;
    down_written_in = ctx.up_written_out;
    down_out = ctx.up_in;
    down_written_out = ctx.up_written_in;
  }

  for (i = 0; i < ac->num; i++) {
    pthread_join(ts[i], NULL);
  }
  for (i = 0; i < ac->num; i++) {
    actor_ctx ctx = ctxs[i];
    if (ctx.up_in) { free(ctx.up_in); free(ctx.up_written_in); free(ctx.up_out); free(ctx.up_written_out); }
  }
}
