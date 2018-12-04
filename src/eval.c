#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
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

/*
  We’re using direct threading here. Basically, we avoid looping and case
  dispatch by using a computed GOTO instead. This comes at the cost of using a
  GNU extension, but I love those anyway.
*/
void* eval(void* arg) {
#define DISPATCH() { c = ctx->code[i++]; goto *dispatch_table[c.code]; }
  int i = 0;
  unsigned int h = 0;
  unsigned int t[TAPE_LEN];
  actor_ctx* ctx = (actor_ctx*) arg;
  bytecode c;
  static void* dispatch_table[] = {
    &&do_zero, &&do_inc, &&do_dec, &&do_fwd, &&do_bck, &&do_prn, &&do_read,
    &&do_startl, &&do_endl, &&do_send, &&do_recv, &&do_move_ptr, &&do_move_data,
    &&do_halt
  };

  for (int idx = 0; idx < TAPE_LEN; idx++) t[idx] = 0;

  DISPATCH();
do_zero: t[h] = 0; DISPATCH();
do_inc: t[h] += c.arg; DISPATCH();
do_dec: t[h] -= c.arg; DISPATCH();
#ifdef NO_WRAP
do_fwd: h += c.arg; DISPATCH();
do_bck: h -= c.arg; DISPATCH();
#else
// modulo would be prettier here, but slows the code down by A LOT; somehow
// the C compilers can’t optimize uncoditional modulos here
do_fwd: h += c.arg; if (h >= TAPE_LEN-1) h %=TAPE_LEN; DISPATCH();
do_bck: h -= c.arg; if (h < 0) h %= TAPE_LEN; DISPATCH();
#endif
do_prn: printf("%c", t[h]); DISPATCH();
do_read: scanf("%c", (char*)&t[h]); DISPATCH();
do_startl: if(!t[h]) i = c.arg; DISPATCH();
do_endl: if(t[h]) i = c.arg; DISPATCH();

do_send:
  if (c.arg) {
    if (!ctx->up_out) {
      fprintf(stderr,
        "Actor %d tried to write to non-existant up channel, ignoring.",
        ctx->num
      );
      DISPATCH();
    }
    *ctx->up_out = t[h];
    *ctx->up_written_out = 1;
    DISPATCH();
  } else {
    if (!ctx->down_out) {
      fprintf(stderr,
        "Actor %d tried to write to non-existant down channel, ignoring.",
        ctx->num
      );
      DISPATCH();
    }
    *ctx->down_out = t[h];
    *ctx->down_written_out = 1;
    DISPATCH();
  }

do_recv:
  while ((!ctx->up_written_in || !(*ctx->up_written_in)) &&
         (!ctx->down_written_in || !(*ctx->down_written_in))) usleep(100);
  if (ctx->up_written_in && *ctx->up_written_in) {
    t[h] = *ctx->up_in;
    *ctx->up_written_in = 0;
  } else if (ctx->down_written_in && *ctx->down_written_in) {
    t[h] = *ctx->down_in;
    *ctx->down_written_in = 0;
  }
  DISPATCH();

do_move_ptr: while (t[h]) h+=c.arg; DISPATCH();

do_move_data:
  if (t[h]) {
    t[h+c.arg] += t[h];
    t[h] = 0;
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
      eval(&ctx);
      free(ctx.up_in);
      free(ctx.up_written_in);
      free(ctx.up_out);
      free(ctx.up_written_out);
      return;
    } else {
      pthread_create(&ts[i], NULL, eval, &ctxs[i]);
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
