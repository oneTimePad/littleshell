#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include "processmanager.h"
#include "internal.h"
#include "errors.h"


/**
Managers process data structure creation and clean up
**/


/**
* initialize process table
* pman: ptr to process manager structure
* returns: status
**/
_BOOL job_manager_init(JMANAGER* pman){
    if(pman == NULL) return FALSE;
    int i =0;
    for(;i<MAX_PROCESSES;i++){
      pman->processpids[i]=-1;
    }
    pman->recent_foreground_status = 0;


    //set tty for processes
    int my_pid = getpid();
    //put shell in foreground
    tcsetpgrp(0,my_pid);
    pman->foreground_group=my_pid;

    return TRUE;
}


/**
* returns an empty job's INDEX, add 1 to get the job number
* jman: job manager
* returns: job index, -1 on error
**/
int find_empty_job(JMANAGER *jman){
  int job = jman->current_job;
  while(jobsnames[job++][0]!='\0'){
    if(job == jman->current_job){
      errno = ENOMEM;
      return -1;
    }
    else if(job == MAX_JOBS){
      job = 0;
    }
  }
  jman->current_job = job;
  return job;
}



/**
* fetch unique job num this processes is associated with
**/
int  process_to_job(JMANAGER *jman,pid_t pid){
    long index = (long)pid -(long)jman->procs.lowest_pid;
    return jman->processes[index];
}




/**
* initializes process in process table
* pman: ptr to process manager
* name: name of process image
* pid: process id
* ground: process is fore or background
* returns: status of success
**/
int job_init(JMANAGER *jman,EMBRYO *embryo,pid_t pid){
  int job = (int)((long)jman->current_job - (long)jman->lowest_pid);
  //loop back around if job not found
  if(job >= MAX_JOBS){
    job = 0;
    while(jobsnames[job++][0]!='\0'){
        if(job == MAX_JOBS) //all job entries are in use
          return -1; //should never happen
    }
    jman->current_job = job - 1;
  }

  //construct the job name string
  if(jman->jobnames[job][0] == '\0'){
    char * str =embryo->start_job_name;
    int num_comp = embryo->num_components_job_name;
    int num  = 0;
    while(num<num_comp){
      if(strlen(str)+3>=MAX_JOB_NAME){errno = ENOMEM; return FALSE;}
      int i;
      strcat(jman->jobsnames[job],((i=inInternal(str)) ? internals[i] : str);
      if(num+1>=num_comp)
        break;
      strcat(jman->jobsnames[job]," ");
      str = str + strlen(str);
      num++;
    }

  }
  else{
    jman->p
  }

  //set process image name
  if(strlen(embryo->program)+1> MAX_PROCESSES)
    return -1;
  strcpy(pman->processnames[i],embryo->program);
  pman->processpids[i] = pid;
  pman->suspendedstatus[i] = FALSE;
  if(!*embryo->background && pman->foreground_group == -1)
    return -1;
  //might be the first process in background group
  if(*embryo->background && pman->background_group == -1){
    pman->background_group = pid;
  }
  //set process job group
  if(setpgid(pid,(*embryo->background) ? pman->background_group : pman->foreground_group) == -1)
    return -1;



  return i;
}




/**
* clean up process entry on death
* pman: ptr to process manager
* proc_index: index in process table
**/
_BOOL process_destroy(PMANAGER *pman,int proc_index){

  memset(pman->processnames[proc_index],0,MAX_PROCESS_NAME);
  pman->processpids[proc_index] = -1;
  pman->err[proc_index] = 0;
  return TRUE;
}


/**
* converts embryos into actual processes
* pman: manager for processes
* embryos: list of embryos
* num_embryos: number of embryos to be created
* err_ptr: used to return child errno to caller
* returns: status if false and, if errno is 0, check err_ptr(child error), else the parent had an error
**/
_BOOL jobs_init(JMANAGER *jman,EMBRYO *embryos,EMBRYO_INFO *info,size_t num_embryos){
  int err_ind =0 ;
  errno = 0;

  //create blockset for waiting, emptyset for restoring child
  sigset_t blockset,emptyset;
  if(sigemptyset(&blockset) == -1)
    return FALSE;
  if(sigemptyset(&emptyset) == -1)
    return FALSE;
  if(sigaddset(&blockset,SYNC_SIG) == -1)
    return FALSE;
  if(sigaddset(&blockset,FAIL_SIG) == -1)
    return FALSE;


  int index =0; //current embryo
  int fork_seq = cur_job_num; //current set of forked processes

  while(1){
    //used to communicate child errno and synchronize
    int pipes[2];
    if(pipe(pipes) == -1)
      return FALSE;

    pid_t pid;
    switch ((pid=fork())) {
      //internal error
      case -1:{
        return FALSE;
        break;
      }
      //child
      case 0:{
        //close read end (not needed)
        if(close(pipes[0]) == -1)
          chldExit(errno);
        //make write end CLOEXEC to determine if execed worked
        if(fcntl(pipes[1],F_SETFD,FD_CLOEXEC) == -1)
          chldExit(errno);

        //duplicate stdin if necessary
        int fd_in;
        if((fd_in = embryos[index].p_stdin)!=-1 && fd_in!=STDIN_FILENO){
          if(dup2(fd_in,STDIN_FILENO) == -1)
            chldExit(errno);
          if(close(fd_in) == -1)
            chldExit(errno);
        }

        //duplicate stdout if necessary
        int fd_out;
        if((fd_out = embryos[index].p_stdout)!=-1 && fd_out!=STDOUT_FILENO){
          if(dup2(fd_out,STDOUT_FILENO) == -1)
            chldExit(errno);
          if(close(fd_out) == -1)
            chldExit(errno);
        }

        //create the args list
        char *args[embryos[index].num_args];
        args[0] = embryos[index].program;
        int args_index =1;
        char *arguments = embryos[index].arguments;
        for(;args_index<embryos[index].num_args;args_index++){
          args[args_index] = arguments;
          arguments = arguments+strlen(arguments)+1;
        }
        args[args_index] = NULL;

        //sigaction for dfl actions
        struct sigaction dfl_action;
        dfl_action.sa_handler = SIG_DFL;
        if(sigemptyset(&dfl_action.sa_mask) == -1)
          chldExit(errno);
        dfl_action.sa_flags = 0;

        //sync with parent
        union sigval val;
        val.sival_int = -1; //unused
        sigqueue(getppid(),SYNC_SIG,val);

        //wait for parent to finish process entry creation
        siginfo_t info;
        sigwaitinfo(&blockset,&info);


        if(info.si_signo != SYNC_SIG)
          chldPipeExit(pipes[1],-1);//something very very...bad happened

        //restore signal mask
        if(sigprocmask(SIG_SETMASK,&emptyset,NULL) == -1)
          chldPipeExit(pipes[1],errno);
        //restore signal dispositions
        if(sigaction(SIGINT,&dfl_action,NULL) == -1)
          chldPipeExit(pipes[1],errno);
        if(sigaction(SIGQUIT,&dfl_action,NULL) == -1)
          chldPipeExit(pipes[1],errno);
        if(sigaction(SIGTSTP,&dfl_action,NULL) == -1)
          chldPipeExit(pipes[1],errno);

        if(!embryos[index].internal_command){
          //exec
          execv(args[0],args);
          //exec failed notify parent and exit
          chldPipeExit(pipes[1],errno);
        }
        else{
          //run internal command
          errno = 0;
          if(!execute_internal(pipes[1],embryos[index].internal_key,pman,args))
            chldPipeExit(pipes[1],errno);

        }

        break;
      }
      //parent
      default:{

        //whether this part happens before or after child doesn't matter
        //close unused write end
        if(close(pipes[1]) == -1){
          kill(pid,SIGKILL);
          return FALSE;
        }
        //close unused fd
        int fd_in;
        if((fd_in=embryos[index].p_stdin) !=-1 && fd_in != STDIN_FILENO){
          if(close(fd_in) == -1){
            kill(pid,SIGKILL);
            return FALSE;
          }
        }
        //close unused fd
        int fd_out;
        if((fd_out=embryos[index].p_stdout) !=-1 && fd_out != STDOUT_FILENO){
          if(close(fd_out) == -1){
            kill(pid,SIGKILL);
            return FALSE;
          }
        }

        ///wait for child to do preliminary setups
        //protect against race
        siginfo_t info;
        sigwaitinfo(&blockset,&info);
        //something went wrong, child died
        if(info.si_signo!=SYNC_SIG){
          errno = info.si_int; //give caller errno number
          return FALSE;
        }

        //setup process entry
        int id;
        if((id = job_init(jman,&embryos[index],pid,cur_job_num)) == -1){
          kill(pid,SIGKILL);
          return FALSE;
        }
        //notify child
        if(kill(pid,SYNC_SIG) == -1){
          kill(pid,SIGKILL);
          return FALSE;
        }
        //synchronize again
        //wait for pipe write end to close/or have something
        int err;
        switch (read(pipes[0],&err,sizeof(err))) {
          //something really really bad happened
          case -1:{
            kill(pid,SIGKILL);
            close(pipes[0]);
            return FALSE;
          }
          //exec worked everything is fine
          case 0:{
            if(close(pipes[0]) == -1)
              return FALSE;

            err_ind = 0; //reset if some proc in the current fork seq succeeded
            break;
          }
          //exec failed errno is in pipe (child died)
          default:{
            if(close(pipes[0]) == -1)
              return FALSE;
            pman->err[id] = err;
            //notify caller of error
            err_ind = fork_seq; //a proc in the current fork seq failed
            break;
          }
        }
        break;
      }
    }


    //if there are more embryos
    if(index+1<num_embryos){
      //is the next embryo in the same fork seq?
      if(embryos[index+1].fork_seq!=fork_seq){
          //if not wait for the current foreground group
          if(!*embryos[index].background){
            if(!process_wait_foreground(pman))
              return FALSE;
          }
          //whether the proc is background or not doesn't matter since having a & before an && is invalid syntax
          if(err_ind != 0) // the last fork_seq needs to succeed to move on
            return FALSE; //if this is true we can't continue
          //update for_seq for next time
          fork_seq++;
      }
    }
    //no more embryos, just wait for the foreground group and return
    else{ //whether there is an error or not we leave
      if(!*embryos[index].background){
        if(!process_wait_foreground(pman))
          return FALSE;

      }

      break;
    }

  }

  return TRUE;
}


/**
* associate process pid with a job
**/.
_BOOL process_init(JMANAGER *jman,pid_t pid){
    if(jman->procs.lowest_pid == 0)
      jman->procs.lowest_pid = pid;
      //pid's always go up, so take the lowest pid and subtract
    long index = (long) pid - (long)jman->procs.lowest_pid;
    jman->procs.processes[index] = jman->current_job;
    return TRUE:
}



/**
* wait on the foreground process group
**/
_BOOL process_wait_foreground(PMANAGER *pman){
  if(pman->foreground_group == -1){errno = EINVAL; return FALSE;} //never happens

  int status;
  pid_t job;
  //wait for all processes in the foreground group
  while((job = waitpid(-1*pman->foreground_group,&status,WUNTRACED))!=-1)
    process_status(pman,job,status,FALSE);
  return TRUE;
}

/**
* looks for process status changes and failures
* pman: ptr to process manager
* returns: status
**/
_BOOL process_reap(PMANAGER *pman){
  int status;
  pid_t job;
  //poll for processes with status changes
  while((job = waitpid(-1,&status,WNOHANG | WUNTRACED | WCONTINUED))!=0 && job!=-1){
      process_status(pman,job,status,TRUE);
  }

  if(errno != ECHILD && errno !=0)
    return FALSE; //something bad happened

  return TRUE;
}

/**
* get the index of process with pid job
* pman: process manager
* job: pid of job to search for
* returns: index on success, -1 on error
**/
int process_search(PMANAGER *pman,pid_t job){
  int i =0;
  while(i++<MAX_PROCESSES&&pman->processpids[i]!=job);
  return (i<MAX_PROCESSES) ? i : -1;
}


/**
* parses processes wait status when it is cleaned up
* pman: process manager
* job: pid of job that completed
* status: status from waiting on that job
* done_print: whether to print (DONE) for finished process
* returns: status
**/
_BOOL process_status(PMANAGER *pman,pid_t job, int status,_BOOL done_print){
  int index = process_search(pman,job);
  if(index == -1) return FALSE;
  errno = 0;

  //if process failed to be started with some error(print it)
  if(pman->err[index]!=0){
    errno = pman->err[index];
    perror(pman->processnames[index]);
    errno = 0;
    process_destroy(pman,index);
    return TRUE;
  }
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
      printf("Killed                  %s",pman->processnames[index]);
    }
    //convert signal to strmsg
    char *str_sig;
    if((str_sig=strsignal(WTERMSIG(status))) == NULL)
      return FALSE;
    else
      printf("%s ",str_sig);
    //notify of core dump
    #ifdef WCOREDUMP
    if(WCOREDUMP(status))
      printf("(core dumped)");
    #endif
    //clean up process entry
    process_destroy(pman,index);

  }
  //if a process was continued without the use of fg
  //then it must be backgrounded
  else if(WIFCONTINUED(status)){
    pman->suspendedstatus[index] = TRUE;
    if(setpgid(job,pman->background_group))
    printf("Continued                %s",pman->processnames[index]);
  }
  //process just exited
  else if(WIFEXITED(status) && done_print){
    printf("DONE                     %s",pman->processnames[index]);
    //clean up process entry
    process_destroy(pman,index);
  }

  printf("\n");



}


/**
* moves job to the foreground
* pman: ptr to process manager
* job: process id of job to move to foreground
* returns: status
**/
_BOOL process_foreground(PMANAGER *pman,pid_t job){
  //get index of this job in pman
  int index = process_search(pman,job);
  if(index == -1) return FALSE; //something very bad

  //get proc job cntrl group
  int pgrp = getpgid(job);

  //if suspended and in the foreground_group
  if(pman->suspendedstatus[index] && pgrp == pman->foreground_group){
    //continue
    if(kill(job,SIGCONT)==-1)
      return FALSE;
  }
  //if in the background group
  else if(pgrp == pman->background_group){
    //change groups
    if(setpgid(job,pman->foreground_group)==-1)
      return FALSE;

    if(kill(job,SIGCONT)==-1)
      return FALSE;
  }
  else
    return FALSE; //invalid operation

  pman->suspendedstatus[index]=FALSE;
  //wait for this process to finish
  if(!process_wait_foreground(pman))
    return FALSE;

  return TRUE;
}


/**
*  moves job to background
* pman: ptr to process manager
* job: process id of job
**/
_BOOL process_background(PMANAGER *pman, pid_t job){
  //get the index of this job in pman
  int index = process_search(pman,job);
  if(index == -1) return FALSE; //something very bad

  //get job cntrl group
  int pgrp = getpgid(job);

  //if suspended and is in the foreground
  if(pman->suspendedstatus[index] && pgrp == pman->foreground_group){
    //move to the background
    if(setpgid(job,pman->background_group) == -1)
      return FALSE;
    //tell it to continue
    if(kill(job,SIGCONT)==-1)
      return FALSE;
    pman->suspendedstatus[index]=FALSE;
  }
  else
    return FALSE; //invalid operation

  return TRUE;
}

/**
* prints out a list of status of process in pman
* pman: ptr to process manager
**/
_BOOL process_dump(PMANAGER *pman){

  int i = 0;
  for(;i<MAX_PROCESSES;i++){
    if(pman->processpids[i]!=-1 && pman->suspendedstatus[i])
      printf("JOB: %d NAME: %s SUSPENDED\n",pman->processpids[i],pman->processnames[i]);
    else if(pman->processpids[i]!=-1 && !pman->suspendedstatus[i])
      printf("JOB: %d NAME: %s RUNNING\n",pman->processpids[i],pman->processnames[i]);
  }
  return TRUE;

}
