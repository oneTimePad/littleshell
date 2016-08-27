#ifndef PATH_H
#define PATH_H
#include <sys/types.h>
#include "bool.h"



#ifdef _GNU_SOURCE
  #include <linux/limits.h>
  #define PATH_LIM PATH_MAX+NAME_MAX+1
#else
  #define PATH_LIM 8012
#endif
#ifndef PATH
  #define PATH "LPATH"
#endif
extern char LPATH[];
_BOOL inPath(char*,char*,size_t);
#endif
