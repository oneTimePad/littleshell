#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "tokenizer.h"



/**
* remove newlines from user input
* str: input from stdin
* returns: new string without newlines
**/
static char* stripNewlines(char* str){
  int strl = strlen(str);
  if(*(str+strl-1)!=0xa)return str;

  char* newStr;
  if((newStr =(char*)malloc(strl-1)) == NULL)
    return NULL;
  strncpy(newStr,str,strl);
  *(newStr+(strl-1))='\0';
  //free(str);
  return newStr;
}

/**
*splits string based on delimeter
*output: split fills this array with an strings separated by '\0',first string is always '\0'
* might contain an extra '\0' that is ignored
**/
static int split(char* output,size_t max, char* input){
    input = stripNewlines(input);
    if(output==NULL || input == NULL){return 0;}
    int index =0;
    int num_commands =0;
    while(*input!='\0'){
      switch (*input) {
        case '<':{
          //bounds check: increase by 10 if needed so we can reduce the number of reallocs
          output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;

          //since there are 2 characters added to output here, we can safely increase output by 10 char's with no overflow
          if(num_commands!=0&&output[index-1]!='\0'){output[index++]='\0';}

          if(*(input+1) == '<'){output[index++] = RDR_SIN_A; input++;}
          else  {output[index++] = RDR_SIN;}
          output[index++] = '\0';
          num_commands++;
          break;
        }
        case '>':{
          output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;
          if(num_commands!=0&&output[index-1]!='\0'){output[index++]='\0';}

          if(*(input+1) == '>'){output[index++] = RDR_SOT_A; input++;}
          else {output[index++] = RDR_SOT;}
          output[index++] = '\0';
          num_commands++;
          break;
        }
        case '&':{
          output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;
          if(num_commands!=0&&output[index-1]!='\0'){output[index++]='\0';}

          if(*(input+1) == '&') {output[index++] = ANDIN; input++;}
          else {output[index++] = BACK_GR;}
          output[index++] = '\0';
          num_commands++;
          break;
        }
        case '|':{
          output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;
          if(num_commands!=0&&output[index-1]!='\0'){output[index++]='\0';}

          output[index++] = PIPE;
          output[index++] = '\0';
          num_commands++;
          break;
        }
        case ' ':{
          if(num_commands!=0&&output[index-1]!='\0'&&output[index-1]!=' '){
            //since one char is added below, we can safely increase output by 10 with no overflow
            output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;
            output[index++]='\0';
          }
          break;
        }
        default:{
          if((num_commands!=0&&output[index-1]=='\0') || num_commands ==0){num_commands++;}
          //since one char is added below we can safely increase output by 10 with no overflow
          output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;
          output[index++] =*input;
          break;
        }
      }
      input++;
    }
    //edge case
    output = (index>=max)? realloc(output,(max+=EXTENS)*sizeof(char)) : output;
    output[index++] = '\0';
    return num_commands;


}


/**
* create token struct
* tkn: intializes to pointer to a token struct (mallocs)
* input: string from user
* size: length of that input
**/
_BOOL initializeTokens(TOKENS* tkn,char * input,int size){
  if(size<=1) return FALSE;
  //allocate token struct
  //safety cap: the max number of tokens is the length of the user's string
  //this is safe since string is null-terminated
  size_t alloc_size = BUFFER*size*sizeof(char);
  if((tkn->cmd_tokens = (char*)malloc(alloc_size*sizeof(char))) == NULL)
    return FALSE;

  int num_commands =0;
  if((num_commands = split(tkn->cmd_tokens,alloc_size,input)) ==0){
    free(tkn->cmd_tokens);
    return FALSE;
  }
  tkn->num_commands=num_commands;
  tkn->current_command=-1;
  tkn->cmd_index = tkn->cmd_tokens;

  return TRUE;
}
/**
* clean up token struct
* tkn: ptr to token struct
**/
void destroyTokens(TOKENS* tkn){
  free(tkn->cmd_tokens);
}

/**
*fetch next token and update
* tkn: ptr to token struct
* returns: token string
**/
char* getToken(TOKENS* tkn,int status){
  if(status == CURR_TOKEN){
    return tkn->cmd_index;
  }
  else if(status ==NEXT_TOKEN){
    if(tkn->num_commands == tkn->current_command) return NULL;
      tkn->cmd_index = tkn->cmd_index+strlen(tkn->cmd_index)+1;
      tkn->current_command++;
      return tkn->cmd_index;
  }
  else {
    return NULL;
  }
}
