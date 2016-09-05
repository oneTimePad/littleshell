#include "errors.h"





/**
* exit and print response to errno
* fct_name: name of fct with error
**/
 void errnoExit(const char* fct_name){

  if(errno == 0) return;
  perror(fct_name);

}




/**
* print a generic error and exit
* format: format string
**/
 void errExit(const char* format,...){
  va_list ap;
  fprintf(stderr,"Error: ");
  va_start(ap,format);
  vfprintf(stderr,format,ap);
  va_end(ap);
  fflush(stderr);
  exit(EXIT_FAILURE);
}

/**
* exit and print usage
* format: format string
**/
  void usageExit(const char* format,...){
  va_list ap;
  fflush(stderr);
  fprintf(stderr,"Usage: ");
  va_start(ap,format);
  vfprintf(stderr,format,ap);
  va_end(ap);
  fflush(stderr);
  exit(EXIT_FAILURE);
}
