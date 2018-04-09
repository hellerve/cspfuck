#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/debug.h"
#include "src/eval.h"
#include "src/parser.h"

int err(const char* str, ...) {
  va_list ap;
  va_start(ap, str);
  fprintf(stderr, str, ap);
  va_end(ap);
  return 1;
}

int main(int argc, char** argv) {
  if (argc != 2) return err("usage: %s <filename>\n", argv[0]);

  FILE* f = fopen(argv[1], "rb");

  if (!f) return err("couldn’t open file %s\n", argv[1]);

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = 0;

  actors* ac = parse(string);
  free(string);

  if (!ac) return err("parentheses don't match.\n");

  //print_actors(ac);

  eval_actors(ac);

  free_actors(ac);

  return 0;
}
