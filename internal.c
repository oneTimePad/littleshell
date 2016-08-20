#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include "internal.h"


static char* help_info[] = {
    "exit: close shell",\
    "jobs: list current processes",\
    "echo $status: print exit status of last foreground job",\
    "fg <job_id>: run job in foreground(continue)",\
    "bg <job_id>: run job in background",\
    NULL

};

inline void shell_exit(PMANAGER* pman,TOKENS* curr_tkn){
  //terminate all processes
  int i =0;
  for(;i<MAX_PROCESSES;i++)
    if(pman->processpids[i]!=-1)
      kill(pman->processpids[i],SIGTERM);

  if(curr_tkn!=NULL)
    destroyTokens(curr_tkn);
  while(wait()!=-1);
  if(errno!=ECHILD && errno != 0)
    errnoExit("wait()");
  exit(EXIT_SUCCESS);

}


static inline _BOOL shell_foreground(PMANAGER* pman,TOKENS* curr_tkn){
  pid_t job = (pid_t)atoll(getTokenNextCommand(curr_tkn));
  if(!process_foreground(pman,job))
    return FALSE;
  return TRUE;
}

static inline _BOOL shell_background(PMANAGER* pman, TOKENS* curr_tkn){
  pid_t job = (pid_t)atoll(getTokenNextCommand(curr_tkn));
  if(!process_background(pman,job))
    return FALSE;
  return TRUE;
}

static inline void shell_echo(PMANAGER* pman){
  pthread_mutex_lock(&pman->mutex);
  pthread_mutex_lock(&stdout_lock);
  printf("%d\n",pman->recent_foreground_status);
  pthread_mutex_unlock(&stdout_lock);
  pthread_mutex_unlock(&pman->mutex);

}


static inline void print_help(const char* format){
   fflush(stdout);
   printf("Below is the following internal commands: \n");
   char** help = help_info;
   for(;*help!=NULL;help++)
    printf(format,*help);
   fflush(stdout);
}


inline short isInternalCommand(char* cmd){
  if(strcmp(cmd,"jobs")==0)
    return JOBS;
  else if(strcmp(cmd,"exit")==0)
    return SHEXIT;
  else if(strcmp(cmd,"fg")==0)
    return FG;
  else if(strcmp(cmd,"bg")==0)
    return BG;
  else if(strcmp(cmd,"echo")==0)
    return ECHO;
  else if(strcmp(cmd,"help")==0)
    return HELP;
  //unrecognized token
  else
    return NONE;
}


inline _BOOL internal_command(short key,PMANAGER* pman, char* cmd,TOKENS* tkns){
  switch(key){
    case JOBS:
        process_dump(pman);
        break;
    case SHEXIT:
        shell_exit(pman,tkns);
        break;
    case FG:
        return shell_foreground(pman,tkns);
        break;
    case BG:
        return shell_background(pman,tkns);
        break;
    case ECHO:
        shell_echo(pman);
        break;
    case HELP:
        print_help("\r%s\n");
        break;
    default:
      return FALSE;
      break;

  }

  return TRUE;

}
