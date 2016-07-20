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
#include "shmhandler.h"


char* f_shm = "./shm.seg";





/**
*determines if string is an internal command
*return a boolean
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
**/
_BOOL isMetaSymbol(char* cmd){
  char* symbols[] = {"|","<",">","<<",">>","&","&&",NULL};
  char** tmp_p = symbols;
  for(;*tmp_p!=NULL;tmp_p++)
    if(strcmp(*tmp_p,cmd)==0)return TRUE;

  return FALSE;
}





int main(){


  struct sigaction action;

  memset(&action,0,sizeof(action));
  action.sa_handler = &bgProcessHandler;


  if(sigaction(SIGCHLD,&action,NULL)<0){
    perror("sigaction");
    exit(1);
  }

  if(access(f_shm,F_OK)!=-1){
    remove(f_shm);
  }

  PMANAGER* ptr;
  if((ptr=mminit(f_shm,(size_t)sizeof(PMANAGER)))==NULL)
    return errno;

  pthread_mutexattr_t attr;
  if(pthread_mutexattr_setpshared(&attr,PTHREAD_PROCESS_SHARED)!=0){
    perror("pthread_mutexattr_setpshared()");
    return errno;
  }

  if(pthread_mutex_init(&(ptr->mutex),&attr)!=0){
    perror("pthread_mutex_init()");
    return errno;
  }
  mmrelease(ptr,sizeof(PMANAGER));
  ptr = NULL;

  int bytes_in=0;
  size_t nbytes=0;
  char *input_buf = NULL;
  while(1){
    printf("%s","LOLZ> ");
    int bytes_read = (int)getline(&input_buf,&nbytes,stdin);
    TOKENS* curr_tkn;

    if(!initializeTokens(&curr_tkn,input_buf,bytes_read)){
      continue;
    }

    char * str;
    while((str=getTokenNextCommand(curr_tkn))!=NULL){
      if(isExecutable(str)){
        execute(str,curr_tkn);
      }
      else{
        printf("%s: command not found\n",str);
      }

    }
    destroyTokens(curr_tkn);
  }

  return 0;


}
