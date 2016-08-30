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


typedef _PROCESSES{
  int processes[MAX_PROCESSES];
  pid_t lowest_pid;
} PROCESSES;

// contains informaiton about all jobs
typedef struct _JMANAGER{
  char jobnames[MAX_JOB_NAME][MAX_JOBS];
  _BOOL suspendedstatus[MAX_JOBS];
  int err[MAX_JOBS];
  int recent_foreground_job_status;
  int current_job;
  PROCESSES procs;
}JMANAGER;




#endif
