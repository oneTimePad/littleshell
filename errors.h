#ifndef _ERRORS_H
#define _ERRORS_H
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include "bool.h"

#ifndef SIG_FCHLD
  #define SIG_FCHLD SIGRTMIN+7
#endif

inline void errnoExit(const char* fct_name);
inline  void usageExit(const char* format,...);
inline void chldKill(int);
void chldPipeExit(int ,int);


#endif
