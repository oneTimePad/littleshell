#ifndef INTERNAL_H
#define INTERNAL_H
#include "bool.h"
#include "processmanager.h"
#include "tokenizer.h"

#define JOBS   1
#define SHEXIT 2
#define FG     3
#define BG     4
#define ECHO   5
#define HELP   6
#define NONE  -1

void shell_init(PMANAGER **);
inline short isInternalCommand(char*);
inline void shell_exit(PMANAGER*,TOKENS*);
inline _BOOL internal_command(short,PMANAGER*, char*,TOKENS*);
#endif
