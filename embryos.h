#ifndef EMBRYO_H
#define EMBRYO_H
#include <limits.h>
#define MAX_PROCESSES 500
#define MAX_PROCESS_NAME 100
#define MAX_ARGUMENT 10
#define MAX_ARG_LEN  ARG_MAX
#define MAX_JOB_NAME 100
#define MAX_JOBS 500

//used for storing info on a process to be created
//an embryo process is one queued to be forked, but not yet forked
typedef struct _EMBRYO_PROCESS{
  char  program[PATH_LIM];

  char  arguments[MAX_ARGUMENT*MAX_ARG_LEN];
  int   num_args;

  int   fork_seq; //unique job id

  int   p_stdin; //values for stdio redirection
  int   p_stdout;
  int   p_pipe_read; //other end of pipe, if pipe is opened

  short internal_key; //used if this embryo refers to a shell internal command

} EMBRYO;


//allows for embryo init to continue where left off
typedef struct _EMBRYO_INFO{
  int cur_proc; //index of current embryo
  char last_sequence; //last prehandler called
  char *err_character; //chracter sequence that caused an error (i.e syntax error or permission error)

  int fork_seq; //current process group for embryos
  char  forkseqname[MAX_JOB_NAME][MAX_JOBS]; // process group/job names for embryos
  _BOOL background[MAX_JOBS]; //status of background/foreground for embryos


} EMBRYO_INFO;


int isSensitive(char *);
_BOOL add_to_job_name(EMBRYO_INFO *,char);
_BOOL embryo_clean(EMBRYO *,EMBRYO_INFO *);
_BOOL embryo_create(EMBRYO *,EMBRYO_INFO *, char *);
_BOOL embryo_arg(EMBRYO *,EMBRYO_INFO *, char *);
_BOOL embryos_init(TOKENS *tkns,EMBRYO* ,size_t, EMBRYO_INFO*);
#endif
