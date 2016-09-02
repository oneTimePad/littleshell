#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "internal.h"
#include "errors.h"
#include "sensitive.h"
#include "embryos.h"

extern  prehandler prehandlers[];
extern  posthandler posthandlers[];

/**
* determines if first char in str is a shell understood character
* returns: TRUE if is, FALSE if not
**/
static char* sensative_characters[] ={
  "","|",">",">>","<","&","&&",NULL
};
int isSensitive(char *str){
  switch (*str) {
    case PIPE:
      return 1;
      break;
    case RDR_SOT:
      return 2;
      break;
    case RDR_SOT_A:
      return 3;
      break;
    case RDR_SIN:
    case RDR_SIN_A:
      return 4;
      break;
    case BACK_GR:
      return 5;
      break;
    case ANDIN:
      return 6;
      break;
    default:
      return 0;
      break;
  }
}


_BOOL add_to_job_name(EMBRYO_INFO *info,char which){
  char *sens_char = sensative_characters[isSensitive(&which)];
  if(strlen(info->forkseqname[info->fork_seq-1])+strlen(sens_char)+2 >MAX_JOB_NAME){
    errno = ENOMEM;
    return FALSE;
  }
  strcat(info->forkseqname[info->fork_seq-1]," ");
  strcat(info->forkseqname[info->fork_seq-1],sens_char);
  return TRUE;
}


/*
* cleans up embryos
* procs: list of embryos created
* info: info struct about the embryos created
*/
_BOOL embryo_clean(EMBRYO *procs,EMBRYO_INFO *info){
  if(info->cur_proc == -1)return TRUE;
  /*
  if(info->pipe_present && procs[info->cur_proc-1].p_pipe_read!=-1){
    if(close(procs[info->cur_proc].p_pipe_read)==-1)
      return FALSE;
    procs[info->cur_proc].p_stdout=-1;
  }*/
  int num_procs = info->cur_proc;
  int index = 0;

  for(;index < num_procs;index++){

    if(procs[index].p_stdout!=-1 && procs[index].p_pipe_read==-1){
      if(close(procs[index].p_stdout) == -1)
        return FALSE;
    }
    if(procs[index].p_stdin!=-1){
      if(close(procs[index].p_stdin) == -1)
        return FALSE;
    }
  }
  return TRUE;
}


/**
* adds a new embryo to the list of embryos
* procs: list of embryos
* info: information about current execution context of embryos_init
* name: name of this embryo
**/
_BOOL embryo_create(EMBRYO *procs,EMBRYO_INFO *info, char *name){
  if(info->cur_proc+1 >= MAX_JOBS ){
    errno = ENOMEM;
    return FALSE;
  }

  EMBRYO * new_proc = &procs[++info->cur_proc]; //retrieve a new proc entry
  new_proc->fork_seq = info->fork_seq; //set the fork sequence

  //starts forming the job name
  //first entry in the job
  //forkseqname is the pre-cursor to jobname
  if(info->cur_proc-1 <0 || procs[info->cur_proc-1].fork_seq != new_proc->fork_seq){
    if(strlen(name)+1 > MAX_JOB_NAME){
      errno = ENOMEM;
      return FALSE;
    }
    strcpy(info->forkseqname[info->fork_seq-1],name);
  }
  //this is not the first entry in the job
  else{
    if(strlen(name)+strlen(info->forkseqname[info->fork_seq-1])+2 > MAX_JOB_NAME){
      errno = ENOMEM;
      return FALSE;
    }
    strcat(info->forkseqname[info->fork_seq-1]," ");
    strcat(info->forkseqname[info->fork_seq-1],name);

  }


  //attempt to get the process name and check if it is in the path if necessary
  if(strlen(name)+1>PATH_LIM){
    info->cur_proc-=1;
    errno = ENOMEM;
    return FALSE;
  }
  short key = NONE;
  new_proc->internal_key = NONE;
  if(((strstr(name,"/")!= NULL) ? strcpy(new_proc->program,name) :( (inPath(name,new_proc->program,PATH_LIM)&& (key=inInternal(name))==NONE) ? new_proc->program : NULL) ) ==NULL){
    //it might be a command internal to the shell
    if(key!=NONE){
      new_proc->internal_key = key;
    }
    else{
        info->cur_proc-=1;
        errno = ENOENT;
        return FALSE;
    }
  }
  //set up args
  new_proc->num_args = 0;
  //set up file-descriptors
  new_proc->p_pipe_read = -1;
  new_proc->p_stdin = -1;
  new_proc->p_stdout = -1;

}

/**
* add arguments to the current embryo, this is a special posthandler
*procs: list of embryos
* info: current context of embryos_init
* arg: the argument to add
**/
_BOOL embryo_arg(EMBRYO *procs,EMBRYO_INFO *info, char *arg){
  //check for overflow
  if(procs[info->cur_proc].num_args >= MAX_ARGUMENT || strlen(arg) + 1 > MAX_ARG_LEN){
    info->cur_proc-=1;
    errno = ENOMEM;
    return FALSE;
  }

  EMBRYO *proc =  &procs[info->cur_proc];

  strcpy(proc->arguments[proc->num_args++],arg);
  return TRUE;

}


/**
* attempts to form processes and their i/o connections from tkns
* tkns: ptr to the TOKENS
* procs: ptr to EMBRYO structs
* info: contains information about where to start( i.e to be used if we are continuing from a previous call to embryos_init)
* returns: status, return FALSE and errno is set to EINVAL for bad syntax or 0 if we don't have enough info (i.e go back to shell)
**/
_BOOL embryos_init(TOKENS *tkns,EMBRYO* procs, EMBRYO_INFO* info){
  if(info == NULL | tkns == NULL || procs == NULL) {errno = EFAULT; return FALSE;}
  char *cur_tkn;
  int which = CURR_TOKEN;

  while((cur_tkn = getToken(tkns,which))!=NULL){
      switch (isSensitive(cur_tkn)) {
        //if not a sensitive character
        case 0:{
          //if there is at least 1 embryo already
          if(info->cur_proc !=-1){
            //call the post handler

            if(posthandlers[(info->last_sequence*-1)](procs,info,cur_tkn)  == FALSE ){
              info->err_character = cur_tkn;
              return FALSE;
            }

          }
          //else we need to create at least one proc
          //since there is no last_sequence yet
          else{
            if(!embryo_create(procs,info,cur_tkn))
              return FALSE;
          }
          break;
        }
        //is a sensitive char
        default:{
          //call the prehandler based on the *cur_tkn, which sensitive char it is
          if(  prehandlers[(*cur_tkn * -1)](procs,info,*cur_tkn)  == FALSE ){
            if(errno == EINVAL)//syntax error
              info->err_character = cur_tkn;
            return FALSE;
          }
          break;
        }
      }
  }

  if(info->last_sequence != '\0'){
    errno = 0;
    return FALSE;
  }

  return TRUE;
}
