#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "bool.h"
#include "tokenizer.h"
#include "executable.h"
#include "processmanager.h"

//used so we can pass more than one arg to thread routine
struct _pman_stdout_lock{
  //process manager
  PMANAGER pman
  //locks stdout allowing for the two threads to synchronize printf
  pthread_mutex_t stdout_lock;
};



/**
*determines if string is an internal command
*returns: status
**/
_BOOL isInternalCommand(char* cmd){
  char* commands[] = {"exit","jobs","echo","fg","bg",NULL};
  char** tmp_p = commands;
  for(;*tmp_p!=NULL;tmp_p++)
    if(strcmp(*tmp_p,cmd)==0)return TRUE;

  return FALSE;
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
* looks for processes to cleanup, called by separate thread
* arg: ptr to struct containg the arguments for this routine
  -> pman: process manager and stdout_lock: lock for printf
**/
void process_clean(struct _pman_stdout_lock* arg){

  while(1){
    process_cleanup(&arg->pman,&arg->stdout_lock);
  }
}



int main(){


  //create structure to hold process manager and lock for stdout
  struct _pman_stdout_lock * ptr = (struct _pman_stdout_lock*)malloc(sizeof(struct _pman_stdout_lock));
  if(ptr == NULL){
    perror("malloc()");
    return errno;
  }

  PMANAGER* pman = &ptr->pman;
  pthread_mutex_t* stdout_lock = &ptr->stdout_lock;
  //initialize the process manager
  if(!process_manager_init(pman)){
    printf("process table initialization failed\n");
    exit(1);
  }
  //initialize the stdout mutex
  if(pthread_mutex_init(stdout_lock,NULL)!=0){
    perror("pthread_mutex_init()");
    return errno;
  }
  //create thread that looks for processes to clean
  pthread_t clean_thread;
  if(pthread_create(&clean_thread,NULL,(void* (*) (void*)) process_clean,ptr)!=0){
    perror("pthread_create()");
    return errno;
  }


//main loop
  while(1){
    //read use input to shell
    int bytes_in=0;
    size_t nbytes=0;
    char *input_buf = NULL;
    printf("%s","LOLZ> ");
    int bytes_read = (int)getline(&input_buf,&nbytes,stdin);
    fflush(stdin);
    TOKENS* curr_tkn;


    if(bytes_read==-1){
      continue;
    }

    //perform tokenization
    if(!initializeTokens(&curr_tkn,input_buf,bytes_read)){
      continue;
    }


    char * str;
    while((str=getTokenNextCommand(curr_tkn))!=NULL){
      //if token is an an executable
      if(isExecutable(str)){
        execute(pman,str,curr_tkn);
      }
      //if token is an internal command
      else if(isInternalCommand(str)){
        if(strcmp(str,"jobs")==0){
          process_dump(pman,stdout_lock);
        }
      }
      //unrecognized token
      else{
        printf("%s: command not found\n",str);
      }

    }
    //clean up
    destroyTokens(curr_tkn);
  }

  return 0;


}
