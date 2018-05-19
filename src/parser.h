#ifndef parser_h
#define parser_h

enum BYTECODES {
  INC,
  DEC,
  FWD,
  BCK,
  PRN,
  READ,
  STARTL,
  ENDL,
  ZERO,

  SEND,
  RECV,
  HALT,
};

typedef struct {
  int code;
  int arg;
} bytecode;

typedef struct {
  int num;
  bytecode** code;
} actors;

actors* parse(char*);
void free_actors(actors*);

#endif
