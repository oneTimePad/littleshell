#ifndef UTILS_H
#define UTILS_H


#include "bool.h"
//max length varies too much on each system, just choose something small
//warning: but might result in overflowing total env size
//#define MAX_ENV_NAME 2048
//#define MAX_VAL_NAME 2048
//#define MALLOC_MAGIC_STRING "AGHI"
//#define MAGIC_STRING_LEN 4

extern char** environ;

#define S_FOK (int*)-2000;
#define S_XOK (int*)-1000;





_BOOL safe_access(const char*, int flags,int*);



#endif
