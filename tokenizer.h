#ifndef _TOKENS
#define _TOKENS
#define SPACE 0x20
#define NULL_TERM 0x0
/**
* wraps the input tokens
**/
typedef struct _TOKENS{
  int current_command,num_commands;
  char ** cmd_tokens;
} TOKENS;
int initializeTokens(TOKENS** tkn,char * input,int size);
void destroyTokens(TOKENS* tkn);
char* testTokenNextCommand(TOKENS* tkn);
char* getTokenNextCommand(TOKENS* tkn);
#endif
