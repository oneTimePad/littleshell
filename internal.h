#ifndef INTERNAL_H
#define INTERNAL_H
#include "bool.h"
#include "jobmanager.h"
#include "tokenizer.h"

#define JOBS   1
#define SHEXIT 2
#define FG     3
#define BG     4
#define ECHO   5
#define HELP   6
#define NONE  -1

#define NO_FORK(key) ((key == JOBS) ? TRUE : (key == FG) ? TRUE : FALSE)              
short inInternal(char *);
_BOOL shell_exit(_BOOL,JMANAGER *,TOKENS *);
_BOOL execute_internal(int ,short,JMANAGER *,char **);

#endif
