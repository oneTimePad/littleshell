#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>




int main(int argc, char* argv[]){
  char in[20];
 int out = fread(in,1,20,stdin);
  printf("stdin %s\n",in);
//  while(1); 
 return 0;

}
