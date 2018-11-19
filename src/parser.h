#ifndef parser_h
#define parser_h

enum BYTECODES {
  ZERO = 0,
  INC,
  DEC,
  FWD,
  BCK,
  PRN,
  READ,
  STARTL,
  ENDL,

  SEND,
  RECV,

  MOVE_PTR,
  MOVE_DATA,

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
