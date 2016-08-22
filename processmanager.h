#ifndef _PROCMAN
#define _PROCMAN
#define MAX_PROCESSES 500
#define MAX_PROCESS_NAME 100
#define MAX_ARGUMENT 10
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include "bool.h"

#ifndef SYNC_SIG
  #define SYNC_SIG SIGUSR1
#endif



extern const char * const sys_siglist[];
typedef enum {FORE , BACK} _GROUND;

//used for storing info on a process to be created
typedef struct _EMBRYO_PROCESS{

  char* arguments[MAX_ARGUMENT];
  int p_stdin;
  int p_stdout;
  _BOOL background;
} EMBRYO;

// manages processes
typedef struct _PMANAGER{
  // contains the name of image process is running
  char processnames[MAX_PROCESS_NAME][MAX_PROCESSES];
  // contains a list of process pid's
  pid_t processpids[MAX_PROCESSES];
  // says if process is currently suspended
  _BOOL suspendedstatus[MAX_PROCESSES];
  //process group ids
  pid_t foreground_group;
  pid_t background_group;
  int fork_sequence;

  //return status of most recent foreground process
  int recent_foreground_status;

} PMANAGER;

_BOOL process_manager_init(PMANAGER*);
_BOOL process_init(PMANAGER*,char*,pid_t, int*, int);
void process_destroy(PMANAGER*,int);
int process_search(pid_t);
void process_wait_foreground(PMANAGER *);
void process_reap(PMANAGER *);
void process_status(PMANAGER *, pid_t, int,_BOOL);
_BOOL process_foreground(PMANAGER *, pid_t);
_BOOL process_background(PMANAGER *, pid_t);
void process_dump(PMANAGER*);


#endif
