
#ifndef _EXEC
#define _EXEC
#include "bool.h"
#include "tokenizer.h"
#include "processmanager.h"
_BOOL isExecutable(char* );
static void process_arguments(char** ,TOKENS* );
_BOOL prepare_process(PMANAGER*,char* ,EMBRYO* ,int, TOKENS*);
_BOOL execute(PMANAGER*,char*, TOKENS*);
#endif
