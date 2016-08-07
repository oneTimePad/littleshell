#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include "executable.h"

#define PATH "LPATH"

/**
* determines if the string points to an executable file
* cmd: token to test
* returns: whether it is an executable
**/
_BOOL isExecutable(char* cmd){
  if(access(cmd,X_OK)==-1) return FALSE;
  return TRUE;
}


/**
* safely determine if ruid has access to this file
* file_name: file to check
* returns: status
**/
_BOOL safe_FXaccess(const char* file_name){
  //determine if it exists, by attempting to open for reading
  uid_t saved_euid = geteuid();
  uid_t saved_egid = getegid();
  uid_t ruid = getuid();
  uid_t rgid = getgid();
  seteuid(ruid);
  setegid(rgid);

  errno =0;
  int fd;
  //can we open for reading using out ruid/rgid?
  if((fd=open(full_path,O_READ))==-1){
    seteuid(saved_eid);
    return FALSE;
  }

  struct stat stats;
  //use fstat to avoid race condition that would occur with stat
  if(fstat(fd,&stat)==-1)
    return FALSE;
  //can we execute it with this ruid?
  if(ruid==stats.st_uid&&stats.st_mode&S_IXUSR){
    if(close(fd)==-1){
      seteuid(saved_euid);
      setegid(saved_egid);
      return FALSE;
    }
    return TRUE;
  }
  //can't, can we execute it with this rgid?
  else if(rgid==stats.st_gid&&stat.st_mode&S_IXGRP){
    if(close(fd)==-1){
      seteuid(saved_euid);
      setegid(saved_egid);
      return FALSE;
    }
    return TRUE;
  }

  if(close(fd)==-1){
    seteuid(saved_euid);
    setegid(saved_egid);
    return FALSE;
  }
  return FALSE; //nope we can't we this file
}




_BOOL isInPath(char *cmd,char* fpath,size_t size){
  char full_path[PATH_LIM];
  char * path_var;
  #ifdef _GNU_SOURCE
  if((path_var=secure_getenv(PATH))==NULL)
    return FALSE;
  #else
  if((path_var=getenv(PATH))==NULL)
    return FALSE;

  int cur_index =0;
  for(;*path_var!='\0';path_var++){
    if(*path==':'){
      cur_index = 0;
      full_path[cur_index]= '\0';
      strncat(full_path,cmd,strlen(cmd));

    }
    full_path[cur_index++] = *path




  if(strlen(full_path)+1>size){
    errno = ENOMEM;
    return FALSE;
  }
  strcpy(fpath,full_path);
  return TRUE;
}

/**
* determine if the string is a meta symbol
* cmd: string to check for meta symbols
* returns: status
**/
_BOOL isMetaSymbol(char* cmd){
  char* symbols[] = {"|","<",">","<<",">>","&","&&",NULL};
  char** tmp_p = symbols;
  for(;*tmp_p!=NULL;tmp_p++)
    if(strcmp(*tmp_p,cmd)==0)return TRUE;

  return FALSE;
}

/**
*extracts arguments for executable
* arguments: a string of arguments to program
* tkns: tkns struct ptr
**/
static void process_arguments(char** arguments,TOKENS* tkns){
  int index=1;
  char* current_token=testTokenNextCommand(tkns);
  //look at the next token
  for(;current_token!=NULL;current_token=testTokenNextCommand(tkns))
    //if it isn't an internal command or meta symbol it's an arg
    if(!isMetaSymbol(current_token)&&index+2<MAX_ARGUMENT)
        arguments[index++]=getTokenNextCommand(tkns);
    else
      break;
  arguments[index] = NULL;
}


/**
* initialiazes a structure containg process information
* pman: ptr to process manager
* process_name : name of process image to execute
* proc: ptr to EMBRYO structure (output)
* pipe_stdin: pointer to a pipe read end
**/
_BOOL prepare_process(PMANAGER* pman,char* process_name,EMBRYO* proc,int pipe_stdin,TOKENS* tkns){

  //the name is the first arg
  proc->arguments[0]=process_name;
  process_arguments(proc->arguments,tkns);
  //look for a meta command next the args or exe
  char* possible_meta="";

  //file descriptor for I/O redirection
  int new_std_in = -1;
  int new_std_out = -1;
  //boolean of whether the process will background
  proc->background = FALSE;
  if(pipe_stdin!=-1)
    new_std_in=pipe_stdin;
  //look for a meta token (except pipe)
  while((possible_meta=testTokenNextCommand(tkns))!=NULL&&isMetaSymbol(possible_meta) && strcmp(possible_meta,"|")!=0 ){

      //if its a & execute it as a background process
      if(strcmp(possible_meta,"&")==0){
        proc->background = TRUE;
        getTokenNextCommand(tkns);
      }
      //if it's a < redirect standard input
      else if(strcmp(possible_meta,"<")==0){

        getTokenNextCommand(tkns);
        char* ioredir ="";
        //get file to open
        if((ioredir=getTokenNextCommand(tkns))==NULL)
          return FALSE;
        if(access(ioredir,R_OK)==-1)
          return FALSE;
        new_std_in = open(ioredir,O_RDONLY);
      }
      //if it's a > redirect standard output
      else if(strcmp(possible_meta,">")==0){
        getTokenNextCommand(tkns);
        char* ioredir ="";
        //open the file to write output too
        if((ioredir=getTokenNextCommand(tkns))==NULL)
          return FALSE;
        //if it doesn't exist create it
        if(access(ioredir,F_OK)==-1&&access(ioredir,W_OK)==-1)
          new_std_out = open(ioredir,O_CREAT | O_RDWR,0666);
        //we don't have write access to the file, but it does exist
        else if(access(ioredir,F_OK)!=-1)
          return FALSE;
        //we are good
        else
          new_std_out = open(ioredir,O_WRONLY);
      }

    }

    proc->p_stdin = new_std_in;
    proc->p_stdout = new_std_out;
    return TRUE;

}





/**
*execute the executable in a new process
*pman: ptr to process manager
*cmd: name of program to execute
*tkns: ptr to tokens struct
*returns: status of success
**/
_BOOL execute(PMANAGER* pman, char* cmd, TOKENS* tkns,int foreground_group,int* background_group){

  //info for all processes to be created
  EMBRYO new_proc[MAX_PROCESSES];

  //prepare info for the first process
  if(!prepare_process(pman,cmd,new_proc,-1,tkns))
    return FALSE;

  //number of processes in the pipe
  int num_procs_in_pipe = 1;
  //while there is a still a pipe
  char* possible_pipe ="";
  while((possible_pipe=testTokenNextCommand(tkns))!=NULL && strcmp(possible_pipe,"|")==0){
    getTokenNextCommand(tkns);
    //if the next program after the pipe is not valid
    char* second_prog ="";
    if((second_prog=testTokenNextCommand(tkns))==NULL || ! isExecutable(second_prog)){
      return FALSE;
    }
    getTokenNextCommand(tkns);
    //create pipe
    int pipe_fd_pipe[2];
    if(pipe(pipe_fd_pipe)==-1)
      return FALSE;
    //set first proc in pipe's stdout to write end of pipe
    new_proc[num_procs_in_pipe-1].p_stdout = pipe_fd_pipe[1];
    //prepare info for process on the other side of the pipe and set its stdin to the read end of the pipe
    prepare_process(pman,second_prog,&new_proc[num_procs_in_pipe++],pipe_fd_pipe[0],tkns);
  }


  //contains pid of processes to be foregrounded (this might be weird)
  int num_foregrounds = 0;
  int foregrounds[MAX_PROCESSES];

  //to avoid race disable sigint and sigtstp
  signal(SIGINT,SIG_BLOCK);
  signal(SIGTSTP,SIG_BLOCK);
  //look through info for all processes to be created
  EMBRYO* embryos = NULL;
  int i =0;
  for(;i<num_procs_in_pipe;i++){
    embryos = new_proc+i;
    //pipe for watching child
    int pipe_ends[2];
    if(pipe(pipe_ends)==-1)
      return FALSE;




    int pid;
    //set child image
    if((pid=fork())==0){
      signal(SIGINT,SIG_DFL);
      signal(SIGTSTP,SIG_DFL);
      //duplicate std files if necessary
      if(embryos->p_stdin!=-1){
        dup2(embryos->p_stdin,0);
        close(embryos->p_stdin);
      }
      if(embryos->p_stdout!=-1){
        dup2(embryos->p_stdout,1);
        close(embryos->p_stdout);
      }
      //close unused end
      close(pipe_ends[0]);

      //this setup is done in the parent and child to avoid race-condition
      //if foreground put it in the foreground group
      if(!embryos->background){
        setpgid(getpid(),foreground_group);

      }
      //if background put it in the background group
      else{
        //this might be the first process in this group
        if(*background_group==-1){
          *background_group = getpid();
        }
        setpgid(getpid(),*background_group);

      }

      //start
      if(execv(embryos->arguments[0],embryos->arguments)!=0)
        return FALSE;

    }
    //parent
    else{
      //clean up files if necessary
      if(embryos->p_stdin!=-1)
        close(embryos->p_stdin);
      if(embryos->p_stdout!=-1)
        close(embryos->p_stdout);

      //initialize process to table
      if(!process_init(pman,embryos->arguments[0],pid,pipe_ends,embryos->background))
        return FALSE;
      //put it in the foreground table if it is not a bg process
      if(!embryos->background){
        foregrounds[num_foregrounds++]=pid;

        setpgid(pid,foreground_group);

      }
      //put process in background group
      else{
        if(*background_group==-1){
          *background_group = pid;
        }
        setpgid(pid,*background_group);

      }
    }

  }
  //start ignoring them again
  signal(SIGINT,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);

  //clean up all foregrounds
  i =0;
  for(;i<num_foregrounds;i++){
      process_trace(pman,foregrounds[i]);
  }

  return TRUE;

}
