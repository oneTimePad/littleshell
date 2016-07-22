#ifndef _TOKENS
#define _TOKENS
#include "bool.h"
#define SPACE 0x20
#define NULL_TERM 0x0
/**
* wraps the input tokens
**/
typedef struct _TOKENS{
  int current_command,num_commands;
  char ** cmd_tokens;
} TOKENS;

_BOOL initializeTokens(TOKENS** ,char * ,int);
void destroyTokens(TOKENS* );
char* testTokenNextCommand(TOKENS*);
char* getTokenNextCommand(TOKENS*);
#endif
