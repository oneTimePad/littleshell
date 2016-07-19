
#include "bool.h"
#include "tokenizer.h"
#ifndef _EXEC
#define _EXEC
_BOOL isExecutable(char* cmd);
static void process_arguments(char** arguments,TOKENS* tkns);
_BOOL execute(char* cmd, TOKENS* tkns);
#endif
