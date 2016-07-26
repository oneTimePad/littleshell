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
  PMANAGER pman;
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


sig_atomic_t sigint_trig = FALSE;
sig_atomic_t sigstp_trig = FALSE;

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

  //ignore termination and suspension
  signal(SIGTSTP,SIG_IGN);
  signal(SIGINT,SIG_IGN);

  //put shell in foreground
  tcsetpgrp(0,getpid());
  pman->foreground_group=getpid();
//main loop
  while(1){


    //read use input to shell
    int bytes_in=0;
    size_t nbytes=0;
    char *input_buf = NULL;
    printf("%s","little> ");

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
        //fetch the background gpid
        int backgpid = -1;
        pthread_mutex_lock(&pman->mutex);
        backgpid = pman->background_group;
        pthread_mutex_unlock(&pman->mutex);

        execute(pman,str,curr_tkn,pman->foreground_group,&backgpid,stdout_lock);
        //edit it since it might have been -1 before
        pthread_mutex_lock(&pman->mutex);
        pman->background_group = backgpid;
        pthread_mutex_unlock(&pman->mutex);
      }
      //if token is an internal command
      else if(isInternalCommand(str)){
        if(strcmp(str,"jobs")==0){
          process_dump(pman,stdout_lock);
        }
        else if(strcmp(str,"exit")==0){
          //terminate all processes
          int i =0;
          for(;i<MAX_PROCESSES;i++){
            if(pman->processpids[i]!=-1){
              kill(pman->processpids[i],SIGINT);
            }
          }
          destroyTokens(curr_tkn);
          exit(0);
        }
        else if(strcmp(str,"fg")==0){

          pid_t job = (pid_t)atoi(getTokenNextCommand(curr_tkn));
          if(!process_foreground(pman,job,stdout_lock)){
            pthread_mutex_lock(stdout_lock);
            printf("Invalid\n");
            pthread_mutex_unlock(stdout_lock);
          }
        }
        else if(strcmp(str,"bg")==0){

          pid_t job = (pid_t)atoi(getTokenNextCommand(curr_tkn));
          if(!process_background(pman,job)){
            pthread_mutex_lock(stdout_lock);
            printf("Invalid\n");
            pthread_mutex_unlock(stdout_lock);
          }
        }

        else if(strcmp(str,"echo")==0){
          if(strcmp(getTokenNextCommand(curr_tkn),"$status")==0){
            pthread_mutex_lock(stdout_lock);
            pthread_mutex_lock(&pman->mutex);
            printf("%d\n",pman->recent_foreground_status);
            pthread_mutex_unlock(&pman->mutex);
            pthread_mutex_unlock(stdout_lock);
          }
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
