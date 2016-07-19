
#include <stdlib.h>
#include <string.h>
#include "bool.h"
#include "tokenizer.h"
#include <stdio.h>



static char* stripNewlines(char* str){
  int strl = strlen(str);
  if(*(str+strl-1)!=0xa)return str;
  //realloc bug here
  char* newStr = (char*)malloc(strl-1);
  strncpy(newStr,str,strl);
  *(newStr+(strl-1))=NULL_TERM;
  //free(str);
  return newStr;
}

/**
*splits string based on delimeter
*return number of strings parsed
**/
static int split(char* output[],char* input,int delimiter){
    input = stripNewlines(input);
    if(output==NULL)return 0;
    int start=-1;
    int end=0;
    int string_index=-1;

    while(1){
        char curr_c = *(input+end);
        if((curr_c ==delimiter || curr_c==NULL_TERM)&& start!=-1){
          string_index++;
          output[string_index] = (char*)malloc(end-start+1);
          //copy extracted token
          strncpy(output[string_index],input+start,end-start);
          *(output[string_index]+end-start) =NULL_TERM;
          start=-1;

        }
        else if(start==-1&&curr_c!=delimiter){
          start=end;
        }
        end++;
        if(curr_c==NULL_TERM){
          output[string_index+1] = NULL;
          return (string_index+1);
        }
    }

}


/**
* create token struct
**/
int initializeTokens(TOKENS** tkn,char * input,int size){
  if(size<=1) return FALSE;

  *tkn = (TOKENS*)malloc(sizeof(TOKENS)+size*(sizeof(char*)));
  (*tkn)->cmd_tokens = (char**)((*tkn)+1);
  int parsed_len =0;
  if((parsed_len = split((*tkn)->cmd_tokens,input,SPACE)) ==0){
    free(*tkn);
    return FALSE;
  }
  (*tkn)->num_commands=parsed_len;
  (*tkn)->current_command=-1;
  return TRUE;
}
/**
* clean up token struct
**/
void destroyTokens(TOKENS* tkn){
  char** cmds = tkn->cmd_tokens;
  for(;*cmds!=NULL;cmds++) free(*cmds);
  free(tkn);
}
/**
* look at the next token but don't update
**/
char* testTokenNextCommand(TOKENS* tkn){
  int next = tkn->current_command+1;
  if( next== tkn->num_commands)return NULL;
  return tkn->cmd_tokens[next];
}
/**
*fetch next token and update
**/
char* getTokenNextCommand(TOKENS* tkn){
  char* ret = testTokenNextCommand(tkn);
  tkn->current_command++;
  return ret;
}
