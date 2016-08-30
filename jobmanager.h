#ifndef _PROCMAN
#define _PROCMAN
#include <limits.h>
#define MAX_PROCESSES 500
#define MAX_PROCESS_NAME 100
#define MAX_ARGUMENT 10
#define MAX_ARG_LEN  ARG_MAX
#include "bool.h"
#include "path.h"
#include "tokenizer.h"
#include <signal.h>

#ifndef SYNC_SIG
  #define SYNC_SIG SIGRTMIN+6
#endif
#ifndef FAIL_SIG
  #define FAIL_SIG SIGRTMIN+7
#endif



extern const char * const sys_siglist[];


// contains informaiton about all jobs
typedef struct _JMANAGER{
  char jobnames[MAX_JOB_NAME][MAX_JOBS];
  _BOOL suspendedstatus[MAX_JOBS];
  int err[MAX_JOBS];
  int recent_foreground_job_status;
  int current_job;
  PROCESSES procs;
}JMANAGER;

typedef _PROCESSES{
  int processes[MAX_PROCESSES];
  pid_t lowest_pid;
} PROCESSES;


//used for embryo processes
_BOOL embryo_init(TOKENS *,EMBRYO *,size_t, EMBRYO_INFO *);
_BOOL embryo_clean(EMBRYO *,EMBRYO_INFO *);

//used for forked processes
_BOOL processes_init(PMANAGER *,EMBRYO *,size_t);
int process_init(PMANAGER *,EMBRYO *,pid_t);
_BOOL process_manager_init(PMANAGER *);
_BOOL process_destroy(PMANAGER *,int);
_BOOL process_wait_foreground(PMANAGER *);
_BOOL process_reap(PMANAGER *);
int   process_search(PMANAGER *,pid_t);
_BOOL process_status(PMANAGER *,pid_t , int,_BOOL);
_BOOL process_foreground(PMANAGER *,pid_t);
_BOOL process_background(PMANAGER *pman, pid_t);
_BOOL process_dump(PMANAGER *);

#endif
