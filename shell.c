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


struct _pman_stdout_lock{
  PMANAGER pman;
  pthread_mutex_t stdout_lock;
};



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


void process_clean(struct _pman_stdout_lock* arg){

  while(1){

    process_cleanup(&arg->pman,&arg->stdout_lock);

  }
}



int main(){




  struct _pman_stdout_lock * ptr = (struct _pman_stdout_lock*)malloc(sizeof(struct _pman_stdout_lock));
  if(ptr == NULL){
    perror("malloc()");
    return errno;
  }

  PMANAGER* pman = &ptr->pman;
  pthread_mutex_t* stdout_lock = &ptr->stdout_lock;

  if(!process_manager_init(pman)){
    printf("process table initialization failed\n");
    exit(1);
  }

  if(pthread_mutex_init(stdout_lock,NULL)!=0){
    perror("pthread_mutex_init()");
    return errno;
  }
  pthread_t clean_thread;
  if(pthread_create(&clean_thread,NULL,(void* (*) (void*)) process_clean,ptr)!=0){
    perror("pthread_create()");
    return errno;
  }








  while(1){
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
    if(!initializeTokens(&curr_tkn,input_buf,bytes_read)){

      continue;

    }


    char * str;
    while((str=getTokenNextCommand(curr_tkn))!=NULL){
      if(isExecutable(str)){
        execute(pman,str,curr_tkn);
      }
      else if(isInternalCommand(str)){
        if(strcmp(str,"jobs")==0){
          process_dump(pman,stdout_lock);
        }
      }
      else{
        printf("%s: command not found\n",str);
      }

    }
    destroyTokens(curr_tkn);

  }

  return 0;


}
