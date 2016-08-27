#include "errors.h"
#include "processmanager.h"




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
* used for a new child process to signal its parent of failure or synchronization
* pipe_end: write end
* value: errno or sync value
**/
inline void chldExit(int value){
  union sigval val;
  val.sival_int = value;
  sigqueue(getppid(),FAIL_SIG,val);
  _exit(EXIT_FAILURE);

}

void chldPipeExit(int pipe_fd,int value){
  write(pipe_fd,&value,sizeof(int));
  close(pipe_fd);
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
