#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "tokenizer.h"




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
  int i;
  char * str;
  for(i=CURR_TOKEN; (str=getToken(&tkns,i))!=NULL; i=NEXT_TOKEN){
    printf("%s",str);
  }
  exit(EXIT_SUCCESS);
}
