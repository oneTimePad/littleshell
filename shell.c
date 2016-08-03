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
#include "errors.h"
#include "internal.h"



  pthread_mutex_t stdout_lock;








/**
* looks for processes to cleanup, called by separate thread
* arg: ptr to struct containg the arguments for this routine
  -> pman: process manager and stdout_lock: lock for printf
**/
void process_clean(PMANAGER* pman){

  while(1){
    process_cleanup(pman);
  }
}








int main(){





  //create structure to hold process manager and lock for stdout
  PMANAGER * pman = (PMANAGER*)malloc(sizeof(PMANAGER));
  if(pman == NULL)
    errnoExit("malloc()");


  //initialize the process manager
  if(!process_manager_init(pman))
    errExit("%s\n","process table initialization failed");

  //initialize the stdout mutex
  if(pthread_mutex_init(&stdout_lock,NULL)!=0)
    errnoExit("pthread_mutex_init()");

  //create thread that looks for processes to clean
  pthread_t clean_thread;
  if(pthread_create(&clean_thread,NULL,(void* (*) (void*)) process_clean,pman)!=0)
    errnoExit("pthread_create()");


  //ignore termination and suspension
  signal(SIGTSTP,SIG_IGN);
  signal(SIGINT,SIG_IGN);
  pid_t my_pid = getpid();
  //put shell in foreground
  tcsetpgrp(0,my_pid);
  pman->foreground_group=my_pid;

  printf("Welcome To littleshell\n \rtype help\n\n");

//main loop
  while(1){


    //read use input to shell

    size_t nbytes=0;
    char *input_buf = NULL;
    printf("%s","little> ");
    fflush(stdout);
    ssize_t bytes_read = getline(&input_buf,&nbytes,stdin);
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

        execute(pman,str,curr_tkn,pman->foreground_group,&backgpid);
        //edit it since it might have been -1 before
        pthread_mutex_lock(&pman->mutex);
        pman->background_group = backgpid;
        pthread_mutex_unlock(&pman->mutex);
        continue;
      }
      //if token is an internal command
      short key;
      if((key=isInternalCommand(str))!=NONE){
        if(!internal_command(key,pman,str,curr_tkn))
          errnoExit("internal_command()");
      }
      //unrecognized token
      else
        printf("%s: command not found\n",str);


    }

    //clean up
    destroyTokens(curr_tkn);
  }

  return 0;


}
