#include "processmanager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

/**
Managers process data structure creation and clean up
**/




/**
* initialize process table
* pman: ptr to process manager structure
* returns: status of success
**/
_BOOL process_manager_init(PMANAGER* pman){
    if(pman == NULL) return FALSE;
    int i =0;
    for(;i<MAX_PROCESSES;i++){
      pman->processpids[i]=-1;
      pman->procspipe[i].fd=-1;
    }
    pman->foreground_group=-1;
    pman->background_group=-1;
    pman->recent_foreground_status = 0;
    if(pthread_mutex_init(&(pman->mutex),NULL)!=0){
      perror("pthread_mutex_init()");
      return FALSE;
    }

    return TRUE;
}


/**
* initializes process in process table
* pman: ptr to process manager
* name: name of process image
* pid: process id
* ground: process is fore or background
* returns: status of success
**/
_BOOL process_init(PMANAGER* pman,char* name,pid_t pid, int* pipe_ends, int ground){
  pthread_mutex_lock(&pman->mutex);
  //look for unused process entry
  int i =0;
  for(;i<MAX_PROCESSES;i++)
    if(pman->processpids[i]==-1)
      break;

  int read_end = pipe_ends[0];
  int write_end = pipe_ends[1];

  close(write_end);
  //set pipe fd to poll on
  pman->procspipe[i].fd = read_end;

  pman->procspipe[i].events = POLLHUP;


  //set process image name
  int name_length = strlen(name);
  strncpy(pman->processnames[i],name,name_length);
  pman->processnames[i][name_length]=0x0;
  pman->processpids[i] = pid;
  pman->groundstatus[i] = ground;

  pthread_mutex_unlock(&pman->mutex);
  return TRUE;
}


void process_trace(PMANAGER* pman,pid_t job,pthread_mutex_t* stdout_lock){
    int status;
  waitpid(job,&status,WUNTRACED);
  pthread_mutex_lock(&pman->mutex);
  pman->recent_foreground_status = status;
  pthread_mutex_unlock(&pman->mutex);
  //if process was suspended
  if(WIFSTOPPED(status)){
    //if it was the cause of ctl-Z
    if(WSTOPSIG(status)==SIGTSTP){
      //set process to paused state
      pthread_mutex_lock(&pman->mutex);
      int j =0;
      for(;j<MAX_PROCESSES;j++){
        if(pman->processpids[j]==job){
          pman->suspendedstatus[j]=TRUE;
          break;
        }
      }
      pthread_mutex_unlock(&pman->mutex);
    }
  }
  //if process was killed by ctl-C
  else if(WIFSIGNALED(status)){
    //print killed message
    pthread_mutex_lock(stdout_lock);
    printf("killed\n");
    pthread_mutex_unlock(stdout_lock);
  }
}


/**
* moves job to the foreground
* pman: ptr to process manager
* job: process id of job to move to foreground
* returns: status
**/
_BOOL process_foreground(PMANAGER* pman,pid_t job,pthread_mutex_t* stdout_lock){
  int i =0;
  for(;i<MAX_PROCESSES;i++)
    if(pman->processpids[i]==job)
      break;
  if(i == MAX_PROCESSES) return FALSE;

  if(pman->suspendedstatus[i]&&pman->groundstatus[i]==FORE){
    kill(job,SIGCONT);
  }
  else if(pman->groundstatus[i]==BACK){
    pthread_mutex_lock(&pman->mutex);
    pman->groundstatus[i]=FORE;
    pthread_mutex_unlock(&pman->mutex);
    setpgid(job,pman->foreground_group);
    kill(job,SIGCONT);
  }
  else
    return FALSE;
  pthread_mutex_lock(&pman->mutex);
  pman->suspendedstatus[i]=FALSE;
  pthread_mutex_unlock(&pman->mutex);
  process_trace(pman,job,stdout_lock);
  return TRUE;
}


/**
*  moves job to background
* pman: ptr to process manager
* job: process id of job
**/
_BOOL process_background(PMANAGER* pman, pid_t job){

  int i =0;
  for(;i<MAX_PROCESSES;i++)
    if(pman->processpids[i]==job)
      break;

  if(i == MAX_PROCESSES) return FALSE;


  if(pman->groundstatus[i]==FORE&&pman->suspendedstatus[i]){
    setpgid(job,pman->background_group);
    kill(job,SIGCONT);
    pthread_mutex_lock(&pman->mutex);
    pman->groundstatus[i] = BACK;
    pman->suspendedstatus[i]=FALSE;
    pthread_mutex_unlock(&pman->mutex);
  }
  else
    return FALSE;

  return TRUE;
}

/**
* clean up process entry on death
* pman: ptr to process manager
* proc_index: index in process table
**/
static void process_destroy(PMANAGER* pman,int proc_index){

  memset(pman->processnames[proc_index],0,MAX_PROCESS_NAME);
  pman->processpids[proc_index] = -1;
  pman->procspipe[proc_index].fd = -1;
  pman->procspipe[proc_index].events=-1;
}


/**
* ran in separate thread. look for dead processes to clean up
* pman: ptr to process manager
**/
void process_cleanup(PMANAGER* pman,pthread_mutex_t* stdout_lock){

    //notified when child closes pipe write end

    int status = poll(pman->procspipe,MAX_PROCESSES,0);

    if(status>0){

      int i =0;
      pthread_mutex_lock(&pman->mutex);
      for(;i<MAX_PROCESSES;i++){

        //if child closed pipe write end
        if(pman->procspipe[i].revents == POLLHUP){

          //clean up
          int status;
          waitpid(pman->processpids[i],&status,WNOHANG);

          // if it is background, send a msg to stdout
          if(pman->groundstatus[i]==BACK){
            pthread_mutex_lock(stdout_lock);
            printf("DONE: %s\n",pman->processnames[i]);
            pthread_mutex_unlock(stdout_lock);
          }
          process_destroy(pman,i);

        }
      }
      pthread_mutex_unlock(&pman->mutex);

    }


}

/**
* prints out a list of active processes
* pman: ptr to process manager
**/
void process_dump(PMANAGER* pman, pthread_mutex_t* stdout_lock){

  pthread_mutex_lock(&pman->mutex);
  int i = 0;
  pthread_mutex_lock(stdout_lock);
  for(;i<MAX_PROCESSES;i++)
    if(pman->processpids[i]!=-1 && pman->suspendedstatus[i])
      printf("JOB: %d NAME: %s SUSPENDED\n",pman->processpids[i],pman->processnames[i]);
    else if(pman->processpids[i]!=-1 && !pman->suspendedstatus[i])
      printf("JOB: %d NAME: %s RUNNING\n",pman->processpids[i],pman->processnames[i]);

  pthread_mutex_unlock(stdout_lock);
  pthread_mutex_unlock(&pman->mutex);
}
