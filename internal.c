#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
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


/**
* determines if is internal command
* returns: key if is, else NONE
**/
short inInternal(char *cmd){
  if(strcmp(cmd,"jobs")==0)
    return JOBS;
  else if(strcmp(cmd,"fg")==0)
    return FG;
  else if(strcmp(cmd,"exit")==0)
    return SHEXIT;
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

/**
* exits shell and clean up
* pman: process manager
* tkns: token manager
* no return
**/
void shell_exit(_BOOL TERM_SHELL,PMANAGER* pman,TOKENS* tkns){
  if(TERM_SHELL){
    //terminate all processes
    int i =0;
    for(;i<MAX_PROCESSES;i++)
      if(pman->processpids[i]!=-1)
        kill(pman->processpids[i],SIGTERM);

    if(tkns!=NULL)
      destroyTokens(tkns);

    //wait for all child processes to die
    while(wait()!=-1);
    if(errno!=ECHILD && errno != 0)
      errnoExit("wait()");
    exit(EXIT_SUCCESS);
  }
  else{
    _exit(EXIT_SUCCESS);
  }
}

/**
* foreground a process
* pman: process manager
* args: string of args
**/
static void shell_foreground(PMANAGER* pman,char **args){
  char *pid;
  _BOOL error = FALSE;
  _BOOL got_pid = FALSE;
  args++;
  while(args!=NULL){
    if(!got_pid)
      pid = *args;
    else{
      fprintf(stderr,"unknown argument %s\n",*args);
      error = TRUE;
    }
    args++;
  }

  if(error)
    _exit(EXIT_FAILURE);
  pid_t job = (pid_t)atoll(pid);
  if(!process_foreground(pman,job)){
    fprintf(stderr,"failed to forground process %ld\n",(long)job);
    _exit(EXIT_FAILURE);
  }
  return _exit(EXIT_SUCCESS);
}

/**
* background a process
* pman: process manager
* args: string of args
**/
static void shell_background(PMANAGER* pman, char **args){
  char *pid;
  _BOOL error = FALSE;
  _BOOL got_pid = FALSE;
  args++;
  while(args!=NULL){
    if(!got_pid)
      pid = *args;
    else{
      fprintf(stderr,"unknown argument %s\n",*args);
      error = TRUE;
    }
    args++;
  }

  if(error)
    _exit(EXIT_FAILURE);
  pid_t job = (pid_t)atoll(pid);
  if(!process_background(pman,job)){
    fprintf(stderr,"failed to background process %ld\n",(long)job);
    _exit(EXIT_FAILURE);
  }
  return _exit(EXIT_SUCCESS);
}

/**
* print wait status of last process that terminated
* pman: process manager
* args: string of args
**/
static void shell_echo(PMANAGER* pman,char **args){
  _BOOL error = FALSE;
  args++;
  while(args!=NULL){
    fprintf(stderr,"unknown argument %s\n",*args);
    error = TRUE;
    args++;
  }
  if(error)
    _exit(EXIT_FAILURE);
  printf("%d\n",pman->recent_foreground_status);
  _exit(EXIT_SUCCESS);
}

/**
* prints help statement
* format: format for help string printing
* args: string of args
**/
static void shell_help(const char* format,char **args){
  _BOOL error = FALSE;
  args++;
  while(args!=NULL){
    fprintf(stderr,"unknown argument %s\n",*args);
    error = TRUE;
    args++;
  }

  if(error)
    _exit(EXIT_FAILURE);

 fflush(stdout);
 printf("Below is the following internal commands: \n");
 char** help = help_info;
 for(;*help!=NULL;help++)
  printf(format,*help);
 _exit(EXIT_SUCCESS);
}

/**
* dump process info
* pman: process manager
* args: string of arguments
**/
static void shell_dump(PMANAGER *pman,char **args){
  _BOOL error = FALSE;
  args++;
  while(args!=NULL){
    fprintf(stderr,"unknown argument %s\n",*args);
    error = TRUE;
    args++;
  }

  if(error)
    _exit(EXIT_FAILURE);
  if(!process_dump(pman))
    _exit(EXIT_FAILURE);
  _exit(EXIT_SUCCESS);
}



/**
* execute internal command in child process
* pipe_end: write end of pipe to notify parent of errnos
* key: indicates the internal command
* pman: process manager
* args: array of args string to command
* returns: status
**/
_BOOL execute_internal(int pipe_end,short key,PMANAGER* pman,char **args){
  if(pman == NULL || args == NULL){errno = EINVAL; return FALSE;}
  switch(key){
    case JOBS:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_dump(pman,args);
        break;
    case FG:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_foreground(pman,args);
        break;
    case BG:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_background(pman,args);
        break;
    case SHEXIT:
      if(close(pipe_end) == -1){
        _exit(EXIT_FAILURE);
      }
      shell_exit(FALSE,NULL,NULL);
    case ECHO:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_echo(pman,args);
        break;
    case HELP:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_help("\r%s\n",args);
        break;
    default:
      errno = EINVAL;
      return FALSE;
      break;

  }

  return TRUE;

}
