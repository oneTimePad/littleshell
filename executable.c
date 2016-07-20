#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "executable.h"
#include "processmanager.h"
#define FORWARD_SLASH 0x2f
#define MAX_ARGUMENT 4000

/**
* determines if the string points to an executable file
**/
_BOOL isExecutable(char* cmd){
  if(access(cmd,X_OK)==-1) return FALSE;
  return TRUE;
}





/**
*extracts arguments for executable
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
**/
_BOOL execute(char* cmd, TOKENS* tkns){
  //set process structure

  OPROCESS proc;
  strncpy(proc.name,cmd,strlen(cmd));
  char* arguments[MAX_ARGUMENT];
  //the name is the first arg
  arguments[0]=cmd;
  process_arguments(arguments,tkns);
  //look for a meta command next the args or exe
  char* possible_meta="";
  int background = FALSE;
  int new_std_in = -1;
  int new_std_out = -1;
  proc.ground = FORE;
  while((possible_meta=testTokenNextCommand(tkns))!=NULL&&isMetaSymbol(possible_meta) ){

      //if its a & execute it as a background process
      if(strcmp(possible_meta,"&")==0){
        //printf("backgrounding %s %d\n",proc->name,proc->pid);
        proc.ground = BACK;
        getTokenNextCommand(tkns);
      }

      else if(strcmp(possible_meta,"<")==0){
        getTokenNextCommand(tkns);
        char* ioredir ="";
        if((ioredir=getTokenNextCommand(tkns))==NULL)
          return FALSE;
        if(access(ioredir,R_OK)==-1)
          return FALSE;

        new_std_in = open(ioredir,O_RDONLY);

      }

      else if(strcmp(possible_meta,">")==0){
        getTokenNextCommand(tkns);
        char* ioredir ="";
        if((ioredir=getTokenNextCommand(tkns))==NULL)
          return FALSE;
        if(access(ioredir,W_OK)==-1)
          new_std_out = open(ioredir,O_CREAT | O_RDWR,0666);
        else
          new_std_out = open(ioredir,O_WRONLY);

      }

  }

  if(!process_init(&proc)){
    perror("process_init()");
    return FALSE;
  }

  int pid;
  //set child image
  if((pid=fork())==0){
    if(new_std_in!=-1)
      dup2(new_std_in,0);
    if(new_std_out!=-1)
      dup2(new_std_out,1);
    execv(cmd,arguments);
    process_destroy(&proc);
    exit(0);
  }
  else
    //if it's not a background process wait on it
      if(!proc.ground)
        while(wait()!=-1);


  return TRUE;

}
