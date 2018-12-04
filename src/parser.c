#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "parser.h"

char* remove_comments(char* inpt) {
  char* str = inpt;
  int ln = strlen(inpt);

  while (*str != '\0') {
    switch (*str) {
      case '+':
      case '-':
      case '>':
      case '<':
      case '.':
      case ',':
      case '[':
      case ']':
      case '^':
      case 'v':
      case 'u':
        break;
      case '\n':
        if (*(str+1) == '\n') { str++; break; }
      default:
        memmove(str, str+1, ln-(str-inpt));
    }
    str++;
  }

  return inpt;
}

char* optimize_zero(char* inpt) {
  char* str = inpt;
  int ln = strlen(inpt);

  while (*str != '\0') {
    if ((!strncmp(str, "[+]", 3))||(!strncmp(str, "[-]", 3))) {
      memmove(str, str+2, ln-(str-inpt));
      *str = '0';
    }
    str++;
  }

  return inpt;
}

actors* parse(char* inpt) {
#define build_op(c, a) {\
  res = realloc(res, sizeof(bytecode)*(++idx));\
  res[idx-1] = (bytecode){.code=c, .arg=a};\
}
#define build_cumulative_op(c) {\
  if (idx > 0 && res[idx-1].code == c) { res[idx-1].arg++; dup++; }\
  else build_op(c, 1);\
}
  int idx = 0;
  int dup = 0;
  int loop_stack[256];
  int loop_depth = 0;
  actors* ac = malloc(sizeof(actors));
  ac->num = 0;
  ac->code = NULL;
  bytecode* res = NULL;
  inpt = remove_comments(inpt);
  inpt = optimize_zero(inpt);

  while (*inpt != '\0') {
    switch (*inpt) {
      case '+': build_cumulative_op(INC); break;
      case '-': build_cumulative_op(DEC); break;
      case '>': build_cumulative_op(FWD); break;
      case '<': build_cumulative_op(BCK); break;
      case '.': build_op(PRN, 0); break;
      case ',': build_op(READ, 0); break;
      case '0': build_cumulative_op(ZERO); break;
      case '[': {
        loop_stack[loop_depth++] = idx;
        build_op(STARTL, 0);
        break;
      }
      case ']': {
        int matching_begin = loop_depth > 0 ? loop_stack[--loop_depth] : -1;
        if (matching_begin < 0) {
          free_actors(ac);
          free(res);
          return NULL;
        }

        build_op(ENDL, matching_begin+1);
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
#undef build_cumulative_op
}

void free_actors(actors* ac) {
  for (int i = 0; i < ac->num; i++) free(ac->code[i]);
  free(ac);
}
