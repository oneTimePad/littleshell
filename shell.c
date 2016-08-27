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
#include "path.h"




volatile sig_atomic_t term_signal = 0;
void term_handler(int sig){
    term_signal = 1;
}

#define MAX_LINE_LEN 100000

int main(){
  PMANAGER pman;
  char line[MAX_LINE_LEN];

  INIT init_opt;
  init_opt.term_handler = &term_handler;
  init_opt.path = LPATH;
  init_opt.pman = &pman;
  init_opt.line = line;
  init_opt.line_size = MAX_LINE_LEN;
  //initialize the shell
  if(!shell_init(&init_opt))
    errnoExit("shell_init()");
  printf("%s\n",line);


//main loop
  while(1){
    if(term_signal){
      shell_exit(pman,NULL);
    }

    //read use input to shell

    size_t nbytes=0;
    char *input_buf = NULL;
    printf("%s",shell_name);
    fflush(stdout);
    ssize_t bytes_read = getline(&input_buf,&nbytes,stdin);
    TOKENS curr_tkn;

    process_reap(pman);

    if(bytes_read==-1){
      continue;
    }

    //perform tokenization
    if(!initializeTokens(&curr_tkn,input_buf,bytes_read)){
      continue;
    }
    if(strstr)
    execute(pman,tkns);


    //clean up

    destroyTokens(&curr_tkn);
    process_reap(pman);
  }

  return 0;


}
