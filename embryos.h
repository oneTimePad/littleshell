#ifndef EMBRYO_H
#define EMBRYO_H


#define ARG -1



//used for storing info on a process to be created
//an embryo process is one queued to be forked, but not yet forked
typedef struct _EMBRYO_PROCESS{
  char  program[PATH_LIM];

  char  arguments[MAX_ARGUMENT*MAX_ARG_LEN];
  int   num_args;

  int   fork_seq; //unique job id

  int   p_stdin;
  int   p_stdout;
  int   p_pipe_read;

  _BOOL *background;

  short internal_key;

} EMBRYO;


//allows for embryo init to continue where left off
typedef struct _EMBRYO_INFO{
  int cur_proc;
  int fork_seq;
  char  forkseqname[MAX_JOB_NAME][MAX_JOBS];
  char last_sequence;
  char err_character;
} EMBRYO_INFO;


#endif
