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

#define ARG -1

extern const char * const sys_siglist[];

//used for storing info on a process to be created
//an embryo process is one queued to be forked, but not yet forked
typedef struct _EMBRYO_PROCESS{
  char  program[PATH_LIM];
  char  arguments[MAX_ARGUMENT*MAX_ARG_LEN];
  char  forkseqname[MAX_JOB_NAME][MAX_JOBS];
  int   num_args;
  int   p_stdin;
  int   p_stdout;
  int   fork_seq; //unique job id

  char  *start_job_name
  int   num_components_job_name;

  int   my_pipe_other;
  _BOOL *background;
  _BOOL internal_command;
  short internal_key;

} EMBRYO;

//allows for embryo init to continue where left off
typedef struct _EMBRYO_INFO{
  int cur_proc;
  _BOOL pipe_present;
  int fork_seq;
  char last_sequence;
  _BOOL continuing;
} EMBRYO_INFO;

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
