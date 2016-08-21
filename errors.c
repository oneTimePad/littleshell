#include "errors.h"




/**
* exit and print response to errno
* fct_name: name of fct with error
**/
inline void errnoExit(const char* fct_name){
  if(errno == 0) return;
  perror(fct_name);
  exit(EXIT_FAILURE);
}



/**
* used for a new child process to signal its parent of failure
* fct_name: func that caused the error
**/
inline void chldExit(){
  union sigval val;
  val.sival_int = errno;
  if(sigqueue(getppid(),SIG_FCHLD,&val)==-1)
      _exit(EXIT_FAILURE); // all hell breaks lose
  _exit(EXIT_FAILURE);
}

/**
* print a generic error and exit
* format: format string
**/
inline void errExit(const char* format,...){
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
inline  void usageExit(const char* format,...){
  va_list ap;
  fflush(stderr);
  fprintf(stderr,"Usage: ");
  va_start(ap,format);
  vfprintf(stderr,format,ap);
  va_end(ap);
  fflush(stderr);
  exit(EXIT_FAILURE);
}
