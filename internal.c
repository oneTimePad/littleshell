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
* jman: job manager
* tkns: token manager
* no return
**/
void shell_exit(_BOOL TERM_SHELL,JMANAGER* jman,TOKENS* tkns){
  if(TERM_SHELL){
    //terminate all processes
    int i =0;
    for(;i<MAX_JOBS;i++)
      if(jman->jobpgrids[i]!=-1){
        kill(jman->jobpgrids[i],SIGHUP);
        kill(jman->jobpgrids[i],SIGCONT);
      }
    if(tkns!=NULL)
      destroyTokens(tkns);

    exit(EXIT_SUCCESS);
  }
  else{
    _exit(EXIT_SUCCESS);
  }
}

/**
* foreground a process
* jman: job manager
* args: string of args
**/
static void shell_foreground(JMANAGER* jman,char **args){
  char *pid;
  _BOOL error = FALSE;
  _BOOL got_pid = FALSE;
  args++;
  while(*args!=NULL){
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
  int job = atoi(pid);
  if(job<=0 || job> MAX_JOBS){
    fprintf(stderr,"no such job\n");
    _exit(EXIT_FAILURE);
  }
  if(!job_ground_change(jman,job,FALSE)){
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
static void shell_background(JMANAGER* jman, char **args){
  char *pid;
  _BOOL error = FALSE;
  _BOOL got_pid = FALSE;
  args++;
  while(*args!=NULL){
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
  int job = atoi(pid);
  if(job<=0 || job> MAX_JOBS){
    fprintf(stderr,"no such job\n");
    _exit(EXIT_FAILURE);
  }
  if(!job_ground_change(jman,job,TRUE)){
    fprintf(stderr,"failed to forground process %ld\n",(long)job);
    _exit(EXIT_FAILURE);
  }
  return _exit(EXIT_SUCCESS);
}

/**
* print wait status of last process that terminated
* pman: process manager
* args: string of args
**/
static void shell_echo(JMANAGER* jman,char **args){
  _BOOL error = FALSE;
  args++;
  while(*args!=NULL){
    fprintf(stderr,"unknown argument %s\n",*args);
    error = TRUE;
    args++;
  }
  if(error)
    _exit(EXIT_FAILURE);
  printf("%d\n",jman->recent_foreground_job_status);
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
  while(*args!=NULL){
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
static void shell_dump(JMANAGER *jman,char **args){
  _BOOL error = FALSE;
  args++;
  while(*args!=NULL){
    fprintf(stderr,"unknown argument %s\n",*args);
    error = TRUE;
    args++;
  }

  if(error)
    _exit(EXIT_FAILURE);
  if(!jobs_dump(jman))
    _exit(EXIT_FAILURE);
  _exit(EXIT_SUCCESS);
}



/**
* execute internal command in child process
* pipe_end: write end of pipe to notify parent of errnos
* key: indicates the internal command
* jman: job manager
* args: array of args string to command
* returns: status
**/
_BOOL execute_internal(int pipe_end,short key,JMANAGER* jman,char **args){
  if(jman == NULL || args == NULL){errno = EINVAL; return FALSE;}
  switch(key){
    case JOBS:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_dump(jman,args);
        break;
    case FG:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_foreground(jman,args);
        break;
    case BG:
        if(close(pipe_end) == -1){
          _exit(EXIT_FAILURE);
        }
        shell_background(jman,args);
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
        shell_echo(jman,args);
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
