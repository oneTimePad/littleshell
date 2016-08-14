#ifndef SETFATTR_H
#define SETFATTR_H

#include "../../bool.h"
#include "../../errors.h"

#define NAME     1
#define VALUE    2
#define REMOVE   4
#define NO_DEREF 8
#define RESTORE  16
#define HELP     32
#define REPLACE  64

#define XATTR_OK 0

typedef union _SETFATTR_OPTIONS{

  struct{
    char n:1;
    char v:1;
    char x:1;
    char d:1;
    char r:1;
    char h:1;
    char R:1;
    char un1:1;
  }bits;
  char byte;
} SXA_OPTIONS;

#endif
