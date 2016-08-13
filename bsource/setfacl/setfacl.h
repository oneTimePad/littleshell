#ifndef SETFACL_H
#define SETFACL_H
#include "../../bool.h"

#define SET         1
#define MODIFY      2
#define REMOVE      4
#define SET_FILE    8
#define MODIFY_FILE 16
#define REMOVE_FILE 32
#define REMOVE_ALL  64
#define REMOVE_DEF  128
#define NO_MASK     256
#define MASK        512
#define DEFAULT     1024
#define RESTORE     2048
#define TEST        4096
#define RECURSIVE   8012
#define LOGICAL     16024
#define PHYSICAL    32048
#define VERSION     64096
#define HELP        128192
#define EOFF        256384
#define STIN        512768






//getopt_long options
typedef union _SETFACL_OPTIONS {

  struct{
    char s:1;
    char m:1;
    char x:1;
    char S:1;
    char M:1;
    char X:1;
    char b:1;
    char k:1;
    char n:1;
    char mask:1;
    char d:1;
    char restore:1;
    char test:1;
    char R:1;
    char L:1;
    char P:1;
    char v:1;
    char h:1;
    char eof:1;
    char stdin:1;
  } bits;

  unsigned int word;

} SFA_OPTIONS;




#endif
