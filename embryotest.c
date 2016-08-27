
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "tokenizer.h"
#include "processmanager.h"



int
main(int argc,char *argv[]){


  size_t nbytes=0;
  char *input_buf = NULL;
  ssize_t bytes_read = getline(&input_buf,&nbytes,stdin);

  TOKENS tkns;
  memset(&tkns,0,sizeof(TOKENS));
  if(!initializeTokens(&tkns,input_buf,bytes_read)){
    exit(1);
  }
  EMBRYO embryos[MAX_PROCESSES];;
  memset(&embryos,0,MAX_PROCESSES*sizeof(EMBRYO));
  EMBRYO_INFO info;
  memset(&info,0,sizeof(EMBRYO_INFO));
  info.cur_proc = -1;
  info.fork_seq = 0;
  info.last_sequence = '\0';
  info.pipe_present = FALSE;
  info.continuing = FALSE;
  if(!embryo_init(&tkns,embryos,MAX_PROCESSES,&info))
    printf("failed\n");
  exit(EXIT_SUCCESS);
}
