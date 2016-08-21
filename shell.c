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
#include <pwd.h>
#include "bool.h"
#include "tokenizer.h"
#include "executable.h"
#include "processmanager.h"
#include "errors.h"
#include "internal.h"




volatile sig_atomic_t term_signal = 0;
void term_handler(int sig){
    term_signal = 1;
}

static char LPATH[] = "LPATH=./bin/:";


int main(){
  PMANAGER *pman;
  shell_init(&pman);

//main loop
  while(1){
    if(term_signal)
      shell_exit(pman,NULL);

    //read use input to shell

    size_t nbytes=0;
    char *input_buf = NULL;
    printf("%s",shell_name);
    fflush(stdout);
    ssize_t bytes_read = getline(&input_buf,&nbytes,stdin);
    TOKENS* curr_tkn;

    process_reap(pman);

    if(bytes_read==-1){
      continue;
    }

    //perform tokenization
    if(!initializeTokens(&curr_tkn,input_buf,bytes_read)){
      continue;
    }

    char file_full_path[PATH_LIM];
    _BOOL in_path;
    char * str;
    while((str=getTokenNextCommand(curr_tkn))!=NULL){

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
    process_reap(pman);
  }

  return 0;


}
