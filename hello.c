#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>




int main(int argc, char* argv[]){
  char in[20];
 // int out = fread(in,1,20,stdin);
//  printf("stdin %s\n",in);
 
  printf("arg1%s\n",argv[0]);
  printf("arg2%s\n",argv[1]);
  printf("arg3%s\n",argv[2]);
//  while(1); 
 return 0;

}
