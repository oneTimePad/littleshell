#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "executable.h"
#include "processmanager.h"
#define FORWARD_SLASH 0x2f
#define MAX_ARGUMENT 4000

extern PMANAGER pman;
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
  PROCESS* proc = &pman.procs[pman.num_procs++];
  proc->pid = pman.num_procs-1;
  proc->status = ACTIVE;

  //get exe canonical name
  int start = 0;
  char* tmp_p = cmd;
  for(;*tmp_p!=NULL_TERM;tmp_p++){
    if(*tmp_p==FORWARD_SLASH)
      start=tmp_p-cmd+1;

  }
  //copy the name as an arg and into process struct
  int buffer_size = strlen(cmd)-start+2;
  char exe_name[buffer_size];
  strncpy(exe_name,tmp_p+start,buffer_size);
  strncpy(proc->name,exe_name,buffer_size);
  char* arguments[MAX_ARGUMENT];
  //the name is the first arg
  arguments[0]=exe_name;
  process_arguments(arguments,tkns);
  //look for a meta command next the args or exe
  char* possible_meta="";
  int background = FALSE;
  int new_std_in = -1;
  int new_std_out = -1;
  while((possible_meta=testTokenNextCommand(tkns))!=NULL&&isMetaSymbol(possible_meta) ){

      //if its a & execute it as a background process
      if(strcmp(possible_meta,"&")==0){
        printf("backgrounding %s %d\n",proc->name,proc->pid);
        background=TRUE;
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
        //printf("%d\n",new_std_out);
      }

  }

  int pid;
  //set child image
  if((pid=fork())==0){
    if(new_std_in!=-1)
      dup2(new_std_in,0);
    if(new_std_out!=-1)
      dup2(new_std_out,1);




    execv(cmd,arguments);
    proc->status = DONE;
    exit(0);
  }
  else{
    //if it's not a background process wait on it
      if(!background){
        while(wait()!=-1);
      }
    }
  return TRUE;

}
