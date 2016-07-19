#ifndef _PROCMAN
#define _PROCMAN
#define MAX_PROCESS_ID 30000000
#define MAX_PROCESS_NAME

typedef enum{ACTIVE,DONE} _PROCESS_STATUS;
typedef enum{BACK,FORE} _GROUND;

typedef struct _PROCESS{
  int pid;
  _PROCESS_STATUS status;
  _GROUND ground;
  char name[MAX_PROCESS_NAME];
} PROCESS;

typedef struct _PMANAGER{
  int num_procs;
  PROCESS procs[MAX_PROCESS_ID];
} PMANAGER;

#endif
