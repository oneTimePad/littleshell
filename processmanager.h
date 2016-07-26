#ifndef _PROCMAN
#define _PROCMAN
#define MAX_PROCESSES 500
#define MAX_PROCESS_NAME 100
#define MAX_ARGUMENT 10
#include <poll.h>
#include <pthread.h>
#include "bool.h"

typedef enum {FORE , BACK} _GROUND;

//used for storing info on a process to be created
typedef struct _EMBRYO_PROCESS{

  char* arguments[MAX_ARGUMENT];
  int p_stdin;
  int p_stdout;
  int background;
} EMBRYO;

// manages processes
typedef struct _PMANAGER{
  // contains the name of image process is running
  char processnames[MAX_PROCESS_NAME][MAX_PROCESSES];
  // contains a list of process pid's
  pid_t processpids[MAX_PROCESSES];
  // list of which processes are background and foreground
  _GROUND groundstatus[MAX_PROCESSES];
  // list of pipe read file descriptors to poll on, determines if child died
  struct pollfd procspipe[MAX_PROCESSES];
  // says if process is currently suspended
  _BOOL suspendedstatus[MAX_PROCESSES];
  //process group ids
  pid_t foreground_group;
  pid_t background_group;

  //return status of most recent foreground process
  int recent_foreground_status;
  //synchronize tables
  pthread_mutex_t mutex;
} PMANAGER;

_BOOL process_manager_init(PMANAGER*);
_BOOL process_init(PMANAGER*,char*,pid_t, int*, int);
static void process_destroy(PMANAGER*,int);
void process_cleanup(PMANAGER*,pthread_mutex_t*);
void process_dump(PMANAGER*,pthread_mutex_t*);


#endif
