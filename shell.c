#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bool.h"
#include "tokenizer.h"
#include "executable.h"
#include "processmanager.h"



PMANAGER pman;


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

  int bytes_in=0;
  size_t nbytes=0;
  char *input_buf = NULL;
  while(1){
    printf("%s","LOLZ> ");
    int bytes_read = (int)getline(&input_buf,&nbytes,stdin);
    //input_buf = stripNewlines(input_buf);
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
