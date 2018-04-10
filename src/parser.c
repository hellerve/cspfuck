#include <stdlib.h>
#include <string.h>

#include "parser.h"

int getmatchbck(char* str, char* end) {
  int last_depth[256];
  int cur_depth = 0;
  char* start = str;

  for (int i = 0; i < 256; i++) last_depth[i] = 0;

  while (str++ < end) {
    switch(*str) {
      case '[':
        last_depth[cur_depth++] = str-start;
        break;
      case ']':
        cur_depth--;
        break;
    }
  }

  if (cur_depth < 0) return -1;

  return last_depth[cur_depth];
}

int getmatchfwd(char* str, char* begin) {
  int cur_depth = 1;

  while (cur_depth) {
    switch(*str) {
      case '[':
        cur_depth++;
        break;
      case ']':
        cur_depth--;
        break;
      case '\0': return -1;
    }
    str++;
  }

  return str-begin;
}

actors* parse(char* inpt) {
#define build_op(c, a) {\
  res = realloc(res, sizeof(bytecode)*(++idx));\
  res[idx-1] = (bytecode){.code=c, .arg=a};\
}
  int idx = 0;
  char* str = inpt;
  actors* ac = malloc(sizeof(actors));
  ac->num = 0;
  ac->code = NULL;
  bytecode* res = NULL;

  while (*inpt != '\0') {
    switch (*inpt) {
      case '+': build_op(INC, 0); break;
      case '-': build_op(DEC, 0); break;
      case '>': build_op(FWD, 0); break;
      case '<': build_op(BCK, 0); break;
      case '.': build_op(PRN, 0); break;
      case ',': build_op(READ, 0); break;
      case '[': {
        int matching_end = getmatchfwd(inpt+1, str);
        if (matching_end < 0) {
          free_actors(ac);
          free(res);
          return NULL;
        }

        build_op(STARTL, matching_end);
        break;
      }
      case ']': {
        int matching_begin = getmatchbck(str, inpt);
        if (matching_begin < 0) {
          free_actors(ac);
          free(res);
          return NULL;
        }

        build_op(ENDL, matching_begin);
        break;
      }

      case '^': build_op(SEND, 0); break;
      case 'v': build_op(SEND, 1); break;
      case 'u': build_op(RECV, 0); break;

      case '\n':
        if (*(++inpt) == '\n') {
          if (res) {
            build_op(HALT, 0);
            ac->code = realloc(ac->code, sizeof(bytecode*)*(++ac->num));
            ac->code[ac->num-1] = res;
            res = NULL;
          }
          idx = 0;
          str = inpt;
        } else if (*inpt == '\0') continue;
        break;
    }
    inpt++;
  }

  if (res) {
    build_op(HALT, 0);
    ac->code = realloc(ac->code, sizeof(bytecode*)*(++ac->num));
    ac->code[ac->num-1] = res;
  }

  return ac;
#undef build_op
}

void free_actors(actors* ac) {
  for (int i = 0; i < ac->num; i++) free(ac->code[i]);
  free(ac);
}
