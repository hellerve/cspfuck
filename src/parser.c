#include <stdlib.h>
#include <string.h>

#include "parser.h"

int getmatchbck(char* str, char* end) {
  int last_depth[256];
  int cur_depth = 0;
  int dup = 0;
  char* start = str;

  for (int i = 0; i < 256; i++) last_depth[i] = 0;

  str--;
  while (str++ < end) {
    switch(*str) {
      case '[':
        last_depth[cur_depth++] = str-start-dup;
        break;
      case ']':
        cur_depth--;
        break;
      case '>':
      case '<':
      case '+':
      case '-': {
        char c = *str;
        while (*(++str)==c) dup++;
        str--;
        break;
      }
    }
  }

  if (cur_depth < 0) return -1;

  return last_depth[cur_depth];
}

int getmatchfwd(char* str, char* begin, int dup) {
  int cur_depth = 1;

  while (cur_depth) {
    switch(*str) {
      case '[':
        cur_depth++;
        break;
      case ']':
        cur_depth--;
        break;
      case '>':
      case '<':
      case '+':
      case '-': {
        char c = *str;
        while (*(++str)==c) dup++;
        str--;
        break;
      }
      case '\0': return -1;
    }
    str++;
  }

  return str-begin-dup;
}

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

#include <stdio.h>

int optimize_loop(bytecode* s, int begin, int end) {
  if (begin < 30) printf("eeeeeend: %d %d\n", begin, end);
  bytecode rep = s[begin];
  if (end-begin == 2 && (rep.code == FWD || rep.code == BCK)) {
    // [>] || [<] == MOVE_PTR
    int narg = rep.code == FWD ? rep.arg : -rep.arg;
    s[begin-1] = (bytecode){.code=MOVE_PTR, .arg=narg};
    return 2;
  } else if (end-begin == 5 && rep.code == DEC && s[begin+3].code == INC &&
             rep.arg == s[begin+3].arg == 1) {
    // [->+<] || [-<+>] == MOVE_DATA
    if (s[begin+2].code == FWD && s[begin+4].code == BCK &&
        s[begin+2].arg == s[begin+4].arg) {
      s[begin+1] = (bytecode){.code=MOVE_DATA, .arg=s[begin+2].arg};
      s[begin+2] = (bytecode){.code=ENDL, .arg=begin};
      s = realloc(s, begin+3);
      return 3;
    } else if (s[begin+2].code == BCK && s[begin+4].code == FWD &&
               s[begin+2].arg == s[begin+4].arg) {
      s[begin+1] = (bytecode){.code=MOVE_DATA, .arg=-s[begin+2].arg};
      s[begin+2] = (bytecode){.code=ENDL, .arg=begin};
      return 3;
    }
  }
  return 0;
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
  int elided = 0;
  char* str = inpt;
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
        int matching_end = getmatchfwd(inpt+1, str, dup);
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

        build_op(ENDL, matching_begin+1-elided);
        //int removed = optimize_loop(res, matching_begin+1, idx+elided);
        //idx -= removed;
        //dup += removed;
        //elided += removed;
        //res = realloc(res, sizeof(bytecode)*idx);
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
          str = inpt+1;
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
