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
#include "processmanager.h"


inline void errnoExit(const char* fct_name);
inline  void usageExit(const char* format,...);
inline void chldExit(int);
void chldPipeExit(int ,int);


#endif
