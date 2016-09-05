#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include "errors.h"
#include "init.h"


/**
*initialize shell signal mask and dispositions
* term_handler: fct ptr to handler for SIGTERM
* returns: status
**/
static _BOOL signals_init(void (*term_handler)(int),void (*chld_handler)(int),void (*hup_handler)(int)){
  //ignore termination and suspension
  struct sigaction ign_action;
  ign_action.sa_handler = SIG_IGN;
  if(sigemptyset(&ign_action.sa_mask) == -1)
    return FALSE;
  ign_action.sa_flags = 0;

  //set dispositions
  if(sigaction(SIGTSTP,&ign_action,NULL) == -1)
    return FALSE;
  if(sigaction(SIGINT, &ign_action,NULL) == -1)
    return FALSE;
  if(sigaction(SIGQUIT,&ign_action,NULL) == -1)
    return FALSE;

  //set term disposition
  struct sigaction handler_action;
  handler_action.sa_flags = 0;
  handler_action.sa_handler = term_handler;
  if(sigemptyset(&handler_action.sa_mask) ==-1)
    return FALSE;
  if(sigaction(SIGTERM,&handler_action,NULL)==-1)
    return FALSE;
  handler_action.sa_handler = chld_handler;
  if(sigaction(SIGCHLD,&handler_action,NULL) == -1)
    return FALSE;
  handler_action.sa_handler = hup_handler;
  if(sigaction(SIGHUP,&handler_action, NULL) == -1)
    return FALSE;
  //block some signals
  sigset_t blockset;
  if(sigemptyset(&blockset) == -1)
    return FALSE;
  if(sigaddset(&blockset,SYNC_SIG)==-1)
    return FALSE;
  if(sigaddset(&blockset,SIGTTOU) == -1)
    return FALSE;
  if(sigprocmask(SIG_BLOCK,&blockset,NULL)==-1)
    return FALSE;

  return TRUE;
}

/**
* set the path for this shell
* path: ptr to environment string for LPATH
* returns: status
**/
static inline _BOOL path_init(char *path){
  if(putenv(path)!=0)
    return FALSE;
  return TRUE;
}

/**
* initialize process manager
* returns: status
**/
static _BOOL jman_init(JMANAGER *jman){
  if(jman == NULL) {errno = EINVAL; return FALSE;}
  if(!job_manager_init(jman))
    return FALSE;
  return TRUE;
}


/**
* initialize the shell `line`, printed whenever waiting for i/o
* line: array to hold string
* line_size: size of array
* returns: status
**/
static _BOOL line_init(char *line, size_t line_size){
    if(line == NULL || line_size <= 0){errno = EINVAL; return FALSE;}

    //get shell owner
    uid_t shell_uid;
    shell_uid = getuid();
    struct passwd* pw;
    if( (pw = getpwuid(shell_uid)) == NULL){
      errnoExit("getpwuid()");
      errExit("%s\n","passwd entry for uid not found");
    }
    char *shell_user = pw->pw_name;

    //get shell hostname
    long host_name_max;
    if((host_name_max = sysconf(_SC_HOST_NAME_MAX)) == -1)
      errnoExit("sysconf()");

    char shell_host[host_name_max+1];
    if(gethostname(shell_host,host_name_max+1)==-1)
      errnoExit("gethostname()");

    //shell title
    const char* shell_title = "little>";

    //calcuate total string length
    long total_len = strlen(shell_host)+strlen(shell_user)+strlen(shell_title)+LEN_AT+LEN_COLON+1;
    if(line_size < total_len){ errno = ENOMEM; return FALSE;}
    //store in caller array
    sprintf(line,"%s@%s:%s",shell_user,shell_host,shell_title);

}

/**
* initialize shell
**/
_BOOL shell_init(INIT *init){
  if(init == NULL){errno = EINVAL; return FALSE;}
  if(!signals_init(init->term_handler,init->chld_handler,init->hup_handler))
    return FALSE;
  if(!path_init(init->path))
    return FALSE;
  if(!jman_init(init->jman))
    return FALSE;
  if(!line_init(init->line,init->line_size))
    return FALSE;

  return TRUE;
}
