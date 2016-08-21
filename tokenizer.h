#ifndef _TOKENS
#define _TOKENS
#include "bool.h"


#define PIPE      (char)-1; // '|'
#define RDR_SIN   (char)-2; // '<'
#define RDR_SOT   (char)-3; // '>'
#define RDR_SIN_A (char)-4; // '<<'
#define RDR_SOT_A (char)-5; // '>>'
#define BACK_GR   (char)-6; // '&'
#define ANDIN     (char)-7; // '&&'


#define CURR_TOKEN (int)1
#define NEXT_TOKEN (int)2

/**
* wraps the input tokens
**/
typedef struct _TOKENS{
  int current_command,num_commands;
  char * cmd_index;
  char * cmd_tokens;
} TOKENS;

_BOOL initializeTokens(TOKENS* ,char * ,int);
void destroyTokens(TOKENS* );
char* getToken(TOKENS* ,int );
#endif
