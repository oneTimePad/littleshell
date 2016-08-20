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
    }
    pman->foreground_group=-1;
    pman->background_group=-1;
    pman->recent_foreground_status = 0;

    sigset_t blockset;
    if(sigemptyset(&blockset) == -1)
      errnoExit("sigemptyset()");
    if(sigaddset(&blockset,SIG_FCHLD)==-1)
      errnoExit("sigaddset()");

    if((pman->sig_fchl_fd=signalfd(-1,&blockset,SFD_NONBLOCK | SFD_CLOEXEC)) ==-1)
      errnoExit("signalfd()");

    if(sigaddset(&blockset,SYNC_SIG)==-1)
      errnoExit("sigaddset()");
    if(sigprocmask(SIG_BLOCK,&blockset,NULL)==-1)
      errnoExit("sigprocmask()");

    pid_t my_pid = getpid();
    //put shell in foreground
    tcsetpgrp(0,my_pid);
    pman->foreground_group=my_pid;

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
_BOOL process_init(PMANAGER* pman,char* name,pid_t pid){

  //look for unused process entry
  int i =-1;
  while((++i)<MAX_PROCESSES && pman->processpids[i]!=-1);

  if(i<MAX_PROCESSES){
    //set process image name
    int name_length = strlen(name);
    strncpy(pman->processnames[i],name,name_length);
    pman->processnames[i][name_length]='\0';
    pman->processpids[i] = pid;
  }
  else
    return FALSE;


  return TRUE;
}

/**
* get the index of process with pid job
* returns index on success, -1 on error
**/
int process_search(pid_t job){
  int i =0;
  while(i++<MAX_PROCESSES&&pman->processpids[i]!=job);
  return (i<MAX_PROCESSES) ? i : -1;
}

void process_wait_foreground(PMANAGER *pman){
  if(pman->foreground_group == -1) //never happens
    errExit("%s\n","no foreground group but foreground processes exist");
  int status;
  pid_t job;
  //wait for all processes in the foreground group
  while((job = waitpid(-1*pman->foreground_group,&status,WUNTRACED))!=-1)
    process_status(pman,job,status,FALSE);
}

/**
* looks for process status changes and failures
* pman: ptr to process manager
**/
void process_reap(PMANAGER *pman){
  int status;
  pid_t job;
  //poll for processes with status changes
  while((job = waitpid(-1,&status,WNOHANG | WUNTRACED | WIFCONTINUED))!=0 && job!=-1){
      process_status(pman,job,status,TRUE);

  }

  if(errno != ECHILD && errno !=0)
    errnoExit("waitpid()");
  struct signalfd_siginfo info;
  memset(&info,0,sizeof(struct signalfd_siginfo));
  //look for child processes that failed to start up and exec
  //these processes signaled RTSIG SIG_FCHLD
  //poll for pending signals
  while(read(pman->sig_fchl_fd,&info,sizeof(struct signalfd_siginfo))== sizeof(struct signalfd_siginfo)){
      if(info.ssi_signo != SIG_FCHLD) errExit("%s\n","unknown error occured while reaping processes"); //never happens
      int32_t pid = info.ssi_int;
      int index = process_search(pid);
      if(index == -1) errExit("%s\n","unknown error occured while reaping processes"); //should never happen
      fprintf(stderr,"%s:%s\n","Failed to create job",pman->processnames[index]);
      //the process failed but it did have a process entry, so remove it
      process_destroy(pman,index);
  }
}


void process_status(PMANAGER* pman,pid_t job, int status,_BOOL done_print){
  int index = process_search(job);
  if(index == -1) return FALSE;
  //if process was suspended
  if(WIFSTOPPED(status)){
    //if it was the cause of ctl-Z
    pman->suspendedstatus[index]=TRUE;
    printf("Stopped                   %s",pman->processnames[index]);
  }
  //if process was killed by a signal
  else if(WIFSIGNALED(status)){
    //if it was SIGKILL, print the killed msg
    if(WTERMSIG(status) == SIGKILL){
      printf("Killed                  %s",pman->processnames[index])
    }
    //convert signal to strmsg
    char *str_sig;
    if((str_sig=strsignal(WTERMSIG(status))) == NULL)
      errnoExit("strsignal()");
    else
      printf("%s ",str_sig);
    //notify of core dump
    #ifdef WCOREDUMP
    if(WCOREDUMP(status))
      printf("(core dumped)");
    #endif

  }
  //if a process was continued without the use of fg
  //then it must be backgrounded
  else if(WIFCONTINUED(status)){
    pman->suspendedstatus[index] = TRUE;
    if(setpgid(job,pman->background_group))
    printf("Continued                %s",pman->processnames[index]);
  }

  else if(WIFEXITED(status) && done_print){
    printf("DONE                     %s",pman->processnames[index]);
  }

  printf("\n");


}


/**
* moves job to the foreground
* pman: ptr to process manager
* job: process id of job to move to foreground
* returns: status
**/
_BOOL process_foreground(PMANAGER* pman,pid_t job){
  int index = process_search(job);
  if(index == -1) return FALSE;


  int pgrp = getpgid(job);

  if(pman->suspendedstatus[i] && pgrp == pman->process_foreground){
    if(kill(job,SIGCONT)==-1)
      errnoExit("kill()");
  }
  else if(pgrp == pman->background_group){
    if(setpgid(job,pman->foreground_group)==-1)
      errnoExit("setpgid()");
    if(kill(job,SIGCONT)==-1)
      errnoExit("kill()");
  }
  else
    return FALSE;

  pman->suspendedstatus[i]=FALSE;
  //wait for this process to finish
  process_wait_foreground(pman);

  return TRUE;
}


/**
*  moves job to background
* pman: ptr to process manager
* job: process id of job
**/
_BOOL process_background(PMANAGER* pman, pid_t job){

  int index = process_search(job);
  if(index == -1) return FALSE;

  int pgrp = getpgid(job);

  if(pman->suspendedstatus[i] && pgrp == pman->foreground_group){
    if(setpgid(job,pman->background_group) == -1)
      errnoExit("setpgid()");
    if(kill(job,SIGCONT)==-1)
      errnoExit("kill()");
    pman->suspendedstatus[i]=FALSE;
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
}


/**
* prints out a list of active processes
* pman: ptr to process manager
**/
void process_dump(PMANAGER* pman){

  pthread_mutex_lock(&pman->mutex);
  int i = 0;
  pthread_mutex_lock(&stdout_lock);
  for(;i<MAX_PROCESSES;i++)
    if(pman->processpids[i]!=-1 && pman->suspendedstatus[i])
      printf("JOB: %d NAME: %s SUSPENDED\n",pman->processpids[i],pman->processnames[i]);
    else if(pman->processpids[i]!=-1 && !pman->suspendedstatus[i])
      printf("JOB: %d NAME: %s RUNNING\n",pman->processpids[i],pman->processnames[i]);

  pthread_mutex_unlock(&stdout_lock);
  pthread_mutex_unlock(&pman->mutex);
}
