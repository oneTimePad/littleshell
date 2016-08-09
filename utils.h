#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "../../errors.h"
#include "../../bool.h"
//max length varies too much on each system, just choose something small
//warning: but might result in overflowing total env size
#define MAX_ENV_NAME 2048
#define MAX_VAL_NAME 2048
#define MALLOC_MAGIC_STRING "AGHI"
#define MAGIC_STRING_LEN 4

extern char** environ;


#define A_FOK 1
#define A_XOK 2
#define A_ROK 4
#define A_WOK 8


int safe_setenv(const char *,const char *, int);
int safe_unsetenv(const char *);
_BOOL safe_access(const char*, int);



#endif
