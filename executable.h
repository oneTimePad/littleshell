
#ifndef _EXEC
#define _EXEC
#include "bool.h"
#include "tokenizer.h"
#include "processmanager.h"

#ifdef _GNU_SOURCE
#include <linux/limits.h>
#define PATH_LIM PATH_MAX+NAME_MAX+1
#else
#define PATH_LIM 8012
#endif


extern pthread_mutex_t stdout_lock;
_BOOL isExecutable(char*,char*, size_t,_BOOL* );
static void process_arguments(char** ,TOKENS* );
static _BOOL isInPath(char*,char*,size_t);
_BOOL prepare_process(PMANAGER*,char* ,EMBRYO* ,int, TOKENS*);
_BOOL execute(PMANAGER*,char*, TOKENS*,int,int*);
#endif
