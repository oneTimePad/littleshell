#ifndef _PROCMAN
#define _PROCMAN
#define MAX_PROCESS_ID 3000
#define MAX_PROCESS_NAME 100
#include <pthread.h>
#include "bool.h"

typedef enum{DONE,ACTIVE} _PROCESS_STATUS;
typedef enum{FORE,BACK} _GROUND;

//describes a process
typedef struct _PROCESS{
  int pid;
  _PROCESS_STATUS status;
  _GROUND ground;
  char name[MAX_PROCESS_NAME];

} PROCESS;
//external process description
typedef struct _OPROCESS{
  int pid;
  _GROUND ground;
  char name[MAX_PROCESS_NAME];
} OPROCESS;

//contains a list of processes
typedef struct _PMANAGER{
  int num_procs;
  PROCESS procs[MAX_PROCESS_ID];
  pthread_mutex_t mutex;
} PMANAGER;

_BOOL process_init(OPROCESS* oproc);
_BOOL process_destroy(OPROCESS* oproc);
_BOOL process_dump(void);
void bgProcessHandler(int sig);

#endif
