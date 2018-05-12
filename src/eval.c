#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "eval.h"

#define TAPE_LEN 30000

typedef struct {
  int* up;
  int* up_written;
  int* down;
  int* down_written;
  bytecode* code;
} actor_ctx;

void* eval(void* arg) {
  int i = 0;
  int h = 0;
  int t[TAPE_LEN];
  actor_ctx* ctx = (actor_ctx*) arg;

  for (int idx = 0; idx < TAPE_LEN; idx++) t[idx] = 0;

  while(1) {
    bytecode c = ctx->code[i++];
    switch (c.code) {
      case INC: t[h]++; break;
      case DEC: t[h]--; break;
      case FWD: h++; break;
      case BCK: h--; break;
      case PRN: printf("%c", t[h]); break;
      case READ: scanf("%c", (char*)&t[h]); break;
      case STARTL: if(!t[h]) i = c.arg; break;
      case ENDL: if(t[h]) i = c.arg; break;

      case SEND: {
        if (c.arg) {
          if (!ctx->up) {
            fputs("Actor tried to write to non-existant up channel, ignoring.", stderr);
            continue;
          }
          *ctx->up = t[h];
          *ctx->up_written = 1;
          break;
        } else {
          if (!ctx->down) {
            fputs("Actor tried to write to non-existant down channel, ignoring.", stderr);
            continue;
          }
          *ctx->down = t[h];
          *ctx->down_written = 1;
          break;
        }
      }
      case RECV: {
        while ((ctx->up_written && !(*ctx->up_written)) &&
               (ctx->down_written && !(*ctx->down_written))) usleep(100);
        if (ctx->up_written && *ctx->up_written) {
          t[h] = *ctx->up;
          *ctx->up_written = 0;
        } else if (ctx->down_written && *ctx->down_written) {
          t[h] = *ctx->down;
          *ctx->down_written = 0;
        }
        break;
      }

      case HALT: return NULL;
    }
  }
}

void eval_actors(actors* ac) {
  int i;
  pthread_t ts[ac->num];
  actor_ctx ctxs[ac->num];
  int* down = NULL;
  int* down_written = NULL;
  for (i = 0; i < ac->num; i++) {
    actor_ctx ctx;
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
