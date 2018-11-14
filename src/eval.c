#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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

/*
  We’re using direct threadin here. Basically, we avoid looping and case
  dispatch by using a computed GOTO instead. This comes at the cost of using a
  GNU extension, but I love those anyway.
*/
void* eval(void* arg) {
#define DISPATCH() { c = ctx->code[i++]; goto *dispatch_table[c.code]; }
  int i = 0;
  int h = 0;
  unsigned int t[TAPE_LEN];
  actor_ctx* ctx = (actor_ctx*) arg;
  bytecode c;
  static void* dispatch_table[] = {
    &&do_zero, &&do_inc, &&do_dec, &&do_fwd, &&do_bck, &&do_prn, &&do_read,
    &&do_startl, &&do_endl, &&do_send, &&do_recv, &&do_halt
  };

  for (int idx = 0; idx < TAPE_LEN; idx++) t[idx] = 0;

  DISPATCH();
do_zero: t[h] = 0; DISPATCH();
do_inc: t[h]++; DISPATCH();
do_dec: t[h]--; DISPATCH();
do_fwd: h = h < TAPE_LEN-1 ? h+1 : 0; DISPATCH();
do_bck: h = h > -1 ? h-1 : TAPE_LEN-1; DISPATCH();
do_prn: printf("%c", t[h]); DISPATCH();
do_read: scanf("%c", (char*)&t[h]); DISPATCH();
do_startl: if(!t[h]) i = c.arg; DISPATCH();
do_endl: if(t[h]) i = c.arg; DISPATCH();

do_send:
  if (c.arg) {
    if (!ctx->up) {
      fputs("Actor tried to write to non-existant up channel, ignoring.", stderr);
      DISPATCH();
    }
    *ctx->up = t[h];
    *ctx->up_written = 1;
    DISPATCH();
  } else {
    if (!ctx->down) {
      fputs("Actor tried to write to non-existant down channel, ignoring.", stderr);
      DISPATCH();
    }
    *ctx->down = t[h];
    *ctx->down_written = 1;
    DISPATCH();
  }

do_recv:
  while ((ctx->up_written && !(*ctx->up_written)) &&
         (ctx->down_written && !(*ctx->down_written))) usleep(100);
  if (ctx->up_written && *ctx->up_written) {
    t[h] = *ctx->up;
    *ctx->up_written = 0;
  } else if (ctx->down_written && *ctx->down_written) {
    t[h] = *ctx->down;
    *ctx->down_written = 0;
  }
  DISPATCH();

do_halt:
  return NULL;
#undef DISPATCH
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
    pthread_create(&ts[i], NULL, eval, &ctxs[i]);
    down = ctx.up;
    down_written = ctx.up_written;
  }
  for (i = 0; i < ac->num; i++) {
    pthread_join(ts[i], NULL);
    actor_ctx ctx = ctxs[i];
    if (ctx.up) { free(ctx.up); free(ctx.up_written); }
  }
}
