#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "executable.h"
#define FORWARD_SLASH 0x2f
#define MAX_ARGUMENT 4000

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
*extracts arguments for executable
* arguments: a string of arguments to program
* tkns: tkns struct ptr
**/
static void process_arguments(char** arguments,TOKENS* tkns){
  int index=0;
  char* current_token=testTokenNextCommand(tkns);
  //look at the next token
  for(;current_token!=NULL;current_token=testTokenNextCommand(tkns))
    //if it isn't an internal command or meta symbol it's an arg
    if(!isMetaSymbol(current_token)&&!isInternalCommand(current_token)&&index+2<4000)
        arguments[index++]=getTokenNextCommand(tkns);
    else
      break;
  arguments[index] = NULL;
}

/**
*execute the executable in a new process
*pman: ptr to process manager
*cmd: name of program to execute
*tkns: ptr to tokens struct
*returns: status of success
**/
_BOOL execute(PMANAGER* pman, char* cmd, TOKENS* tkns){

  char* arguments[MAX_ARGUMENT];
  //the name is the first arg
  arguments[0]=cmd;
  process_arguments(arguments,tkns);
  //look for a meta command next the args or exe
  char* possible_meta="";

  //file descriptor for I/O redirection
  int new_std_in = -1;
  int new_std_out = -1;
  //boolean of whether the process will background
  int background = FALSE;

  //look for a meta token
  while((possible_meta=testTokenNextCommand(tkns))!=NULL&&isMetaSymbol(possible_meta) ){

      //if its a & execute it as a background process
      if(strcmp(possible_meta,"&")==0){
        background = TRUE;
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

  // set up pipe (used for following child process death)
  int pipe_ends[2];
  if(pipe(pipe_ends)==-1){
    perror("pipe()");
    return FALSE;
  }

  int pid;
  //set child image
  if((pid=fork())==0){
    if(new_std_in!=-1)
      dup2(new_std_in,0);
    if(new_std_out!=-1)
      dup2(new_std_out,1);
    //close unused end

    close(pipe_ends[0]);

    //start
    execv(cmd,arguments);
    exit(0);
  }
  else{
    //parent
    //initialize process in table
    if(!process_init(pman,cmd,pid,pipe_ends,background)){
      perror("process_init()");
      return FALSE;
    }
    //if it's not a background proc, wait for it
    if(!background)
        while(wait()!=-1);
  }

  return TRUE;

}
