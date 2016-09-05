#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "shell.h"




volatile sig_atomic_t term_signal = 0;
void term_handler(int sig){
    term_signal = 1;
}

volatile sig_atomic_t chld_signal = 0;
void chld_handler(int sig){
     chld_signal = 1;
}


int main(int argc, char *argv[]){

  JMANAGER jman;
  char line[MAX_LINE_LEN];

  INIT init_opt;
  init_opt.path = LPATH;
  init_opt.jman = &jman;
  init_opt.line = line;
  init_opt.line_size = MAX_LINE_LEN;
  init_opt.term_handler = &term_handler;
  init_opt.chld_handler = &chld_handler;
  init_opt.hup_handler  = &term_handler;
  //initialize the shell
  if(!shell_init(&init_opt))
    errnoExit("shell_init()");



//main loop
  while(1){
    if(term_signal){
      shell_exit(TRUE,&jman,NULL);
    }

    tcsetpgrp(0,getpid());
    //read use input to shell

    size_t nbytes=0;
    char *input_buf = NULL;
    printf("%s",line);
    fflush(stdout);
    ssize_t bytes_read = getline(&input_buf,&nbytes,stdin);
    TOKENS curr_tkn;
    if(chld_signal){
    	job_reap(&jman);
    	chld_signal = 0;
    }
    if(bytes_read==-1){
      continue;
    }

    //perform tokenization
    if(!initializeTokens(&curr_tkn,input_buf,bytes_read)){
      free(input_buf);
      continue;
    }
    free(input_buf);

    if(strcmp(getToken(&curr_tkn,CURR_TOKEN),"exit") == 0){
      shell_exit(TRUE,&jman,&curr_tkn);
    }

    EMBRYO embryos[MAX_EMBRYOS];
    memset(embryos,0,MAX_EMBRYOS*sizeof(EMBRYO));
    EMBRYO_INFO info;
    memset(&info,0,sizeof(EMBRYO_INFO));
    info.cur_proc = -1;
    info.fork_seq =  1;

    _BOOL ret;
    while((ret=execute(&jman,&curr_tkn,embryos,&info)) == FALSE && errno ==0 && info.last_sequence!='\0'){
      destroyTokens(&curr_tkn);
      do{
        errno = 0;
        nbytes = 0;
        memset(&curr_tkn,0,sizeof(TOKENS));
        printf(">");
        fflush(stdout);
        bytes_read = getline(&input_buf,&nbytes,stdin);
        if(bytes_read == -1){
          errnoExit("getline()");
        }
        free(input_buf);
      }
      while(!initializeTokens(&curr_tkn,input_buf,bytes_read));
      if(errno !=0){
        errnoExit("initializeTokens()");
      }
    }

    if(errno !=0 || !ret){ //something went wrong
      if(errno !=0){errnoExit("execute()");}
      else{fprintf(stderr,"shell experienced an error\n");}
      fflush(stderr);
      shell_exit(TRUE,&jman,&curr_tkn);
    }


    //clean up    destroyTokens(&curr_tkn);
    if(chld_signal){
    	job_reap(&jman);
    	chld_signal = 0;
    }
  }

  exit(EXIT_SUCCESS);


}
