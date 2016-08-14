#ifndef GETFATTR_H
#define GETFATTR_H

#include "../../bool.h"
#include "../../errors.h"


#define NAME     1
#define DUMP     2
#define NO_DEREF 4


#define XATTR_OK 0


typedef union _GETFATTR_OPTIONS{

  struct{

    char n:1;
    char d:1;
    char h:1;
    char un1:1;
  } bits;
  int nibble:4;
} GXA_OPTIONS;


#endif
