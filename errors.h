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



 void errnoExit(const char* );
 void usageExit(const char* ,...);
 void errExit(const char*,...);

#endif
