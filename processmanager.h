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
#ifndef SIG_FCHLD
  #define SIG_FCHLD SIGRTMIN+7
#endif

extern const char * const sys_siglist[];
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
  // says if process is currently suspended
  _BOOL suspendedstatus[MAX_PROCESSES];
  //process group ids
  pid_t foreground_group;
  pid_t background_group;

  int sig_fchl_fd;

  //return status of most recent foreground process
  int recent_foreground_status;

} PMANAGER;

_BOOL process_manager_init(PMANAGER*);
_BOOL process_init(PMANAGER*,char*,pid_t, int*, int);
static void process_destroy(PMANAGER*,int);
void process_cleanup(PMANAGER*);
void process_dump(PMANAGER*);


#endif
