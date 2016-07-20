#include "processmanager.h"
#include "shmhandler.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>

extern char* f_shm;

/**
* need to make sure children are cleaned up
**/
void bgProcessHandler(int sig){
    int status;
    //clean up PCB for all dead process fg or bg
    while(waitpid(-1,&status,WNOHANG)!=-1);
}

/**
* initialize a process structure for shell
*args: oproc: external(outer) process structure for describing proc initialization and description
**/
_BOOL process_init(OPROCESS* oproc){
  PMANAGER* ptr;
  //initialize shm map
  if((ptr=(PMANAGER*)mminit(f_shm,sizeof(PMANAGER)))==NULL)
    return FALSE;
  //access shm
  pthread_mutex_lock(&ptr->mutex);
  //created internal process structure
  PROCESS* proc = &ptr->procs[ptr->num_procs++];
  strncpy(proc->name,oproc->name,strlen(oproc->name));
  proc->ground = oproc->ground;
  proc->pid = ptr->num_procs-1;
  proc->status = ACTIVE;
  //set external process structure
  oproc->pid = proc->pid;
  pthread_mutex_unlock(&ptr->mutex);
  mmrelease(ptr,sizeof(PMANAGER));
  return TRUE;
}

_BOOL process_destroy(OPROCESS* oproc){
  PMANAGER* ptr;
  if((ptr=(PMANAGER*)mminit(f_shm,sizeof(PMANAGER)))==NULL)
    return FALSE;
  pthread_mutex_lock(&ptr->mutex);
  PROCESS* proc = &ptr->procs[oproc->pid];
  proc->status =DONE;
  memset(proc->name,0,MAX_PROCESS_NAME);
  pthread_mutex_unlock(&ptr->mutex);
  mmrelease(ptr,sizeof(PMANAGER));
  return TRUE;
}

_BOOL process_dump(void){
  PMANAGER* ptr;
  if((ptr=(PMANAGER*)mminit(f_shm,sizeof(PMANAGER)))==NULL)
    return FALSE;
  pthread_mutex_lock(&ptr->mutex);
  PROCESS* procs = ptr->procs;
  int pid =0;
  for(;pid<MAX_PROCESS_ID;pid++)
    if(procs[pid].status == ACTIVE)
      printf("JOB: %d NAME %s",pid,procs[pid].name);
  pthread_mutex_unlock(&ptr->mutex);
  mmrelease(ptr,sizeof(PMANAGER));
  return TRUE;
}
