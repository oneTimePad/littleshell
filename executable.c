#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "executable.h"



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
        if(safe_access(ioredir,R_OK)==-1)
          return FALSE;
        if((new_std_in = open(ioredir,O_RDONLY)) == -1)
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
    char full_path[PATH_LIM];
    _BOOL in_path;
    if((second_prog=testTokenNextCommand(tkns))==NULL){
      return FALSE;
    }
    second_prog = (in_path) ? full_path : second_prog;
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


  //look through info for all processes to be created
  EMBRYO* embryos = NULL;
  int i =0;
  for(;i<num_procs_in_pipe;i++){
    embryos = new_proc+i;





    pid_t pid;
    //set child image
    switch ((pid=fork())) {
      case -1:{
        return FALSE;
      }
      case 0:{
        sigset_t blockset;
        if(sigemptyset(&blockset)==-1)
          chldExit("sigemptyset()");
        if(sigaddset(&blockset,SIG_FCHLD)==-1)
          chldExit("sigaddset()");
        if(sigprocmask(SIG_UNBLOCK,&blockset,NULL)==-1)
          chldExit("sigprocmask()");

        //duplicate std files if necessary
        if(embryos->p_stdin!=-1 && embryos->p_stdin!=STDIN_FILENO){
          if(dup2(embryos->p_stdin,STDIN_FILENO) == -1)
            chldExit("dup2()");
          if(close(embryos->p_stdin)==-1)
            chldExit("close()");
        }
        if(embryos->p_stdout!=-1 && embryos->p_stdout!=STDOUT_FILENO){
          if(dup2(embryos->p_stdout,STDOUT_FILENO)==-1)
            chldExit("dup2()");
          if(close(embryos->p_stdout)==-1)
            chldExit("close()");
        }


        if(kill(getppid(),SYNC_SIG)==-1)
          chldExit("kill()");



        //overwride parent signal ignoring
        signal(SIGINT,SIG_DFL);
        signal(SIGQUIT,SIG_DFL);
        signal(SIGTSTP,SIG_DFL);

        siginfo_t signal_info;
        if(sigwaitinfo(&blockset,&signal_info)== -1)
          chldExit("sigwaitinfo()");

          //all errors after sigwaitinfo() must be caught by waiting on proc
        if(signal_info.si_signo!= SYNC_SIG)
          chldExit("sigwaitinfo()");


        if(sigaddset(&blockset,SYNC_SIG)==-1)
          chldExit("sigaddset()");
        if(sigprocmask(SIG_UNBLOCK,&blockset,NULL)==-1)
          chldExit("sigprocmask()");
        //give it a clean env for security reasons

        char* envp[MAX_ENV];
        envp[MAX_ENV-1] = NULL;

        //start
        if(execve(embryos->arguments[0],embryos->arguments,envp)==-1)
          chldExit("execve()");

        break;


      }
    //parent
      default:{
        siginfo_t signal_info;
        if(sigaddset(&blockset,SIG_FCHLD)==-1){
          kill(pid,SIGKILL);
          return FALSE;
        }
        if(sigwaitinfo(&blockset,&signal_info)==-1){
          kill(pid,SIGKILL);
          return FALSE;
        }
        if(signal_info.si_signo==SIG_FCHLD){
          fprintf(stderr,"%s\n","new process failed to be created");

          //clean up files if necessary
          if(embryos->p_stdin!=-1){
            if(close(embryos->p_stdin) == -1)
              return FALSE;
          }
          if(embryos->p_stdout!=-1){
            if(close(embryos->p_stdout) == -1)
              return FALSE;
          }
          return FALSE;
        }
        else if(signal_info.si_signo!= SYNC_SIG){
          kill(pid,SIGKILL);
          return FALSE;
        }

        //clean up files if necessary
        if(embryos->p_stdin!=-1){
          if(close(embryos->p_stdin) == -1)
            return FALSE
        }
        if(embryos->p_stdout!=-1){
          if(close(embryos->p_stdout) == -1)
            return FALSE;
        }

        //initialize process to table
        if(!process_init(pman,embryos->arguments[0],pid,pipe_ends,embryos->background))
          return FALSE;
        //put it in the foreground table if it is not a bg process
        if(!embryos->background){
          foregrounds[num_foregrounds++]=pid;

          if(setpgid(pid,foreground_group) == -1)
            return FALSE;

        }
        //put process in background group
        else{
          if(*background_group==-1){
            *background_group = pid;
          }
          if(setpgid(pid,*background_group) == -1)
            return FALSE;
        }

        if(kill(pid,SYNC_SIG)==-1){
          kill(pid,SIGKILL);
          return FALSE;
        }

        break;

      }
    }
  }

  process_wait_foreground(pman);



  return TRUE;

}
