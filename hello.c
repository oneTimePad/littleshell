#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>




int main(int argc, char* argv[]){
  char in[4];
 int out = fread(in,1,4,stdin);
  printf("first_stdin %s\n",in);
  int i;
//  scanf("%d\n",&i); 
 // printf("arg1%s\n",argv[0]);
//  printf("arg2%s\n",argv[1]);
//  printf("arg3%s\n",argv[2]);
//  fwrite("lolz",1,4,stdout);
//  while(1); 
 return 0;

}
