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
Manages job creation and clean up
**/


/**
* initialize process table
* pman: ptr to process manager structure
* returns: status
**/
_BOOL job_manager_init(JMANAGER *jman){
    if(jman == NULL) return FALSE;
    memset(jman,0,sizeof(JMANAGER));

    int i =0;
    for(;i<MAX_JOBS;i++){
      jman->jobpgrids[i]=-1;
    }
    jman->recent_foreground_status = 0;

    return TRUE;
}


/**
* returns an empty job's INDEX+1, add 1 to get the job number
* jman: job manager
* returns: job index +1, -1 on error
**/
int find_empty_job(JMANAGER *jman){
  int job = jman->current_job;
  while(jman->jobpgrids!=-1){
    if(job == jman->current_job){
      errno = ENOMEM;
      return -1;
    }
    else if(job == MAX_JOBS){
      job = 0;
    }
  }
  jman->current_job = job;
  return job+1;
}


/**
* wait on the foreground process group
* jman: job manager
* job: job to wait on
* returns: status
**/
_BOOL job_wait_foreground(JMANAGER *jman, int job){
  if(job == -1){errno = EINVAL; return FALSE;} //never happens
  pid_t pgid = jman->jobpgrids[job-1];
  int status;
  pid_t pid;
  //wait for all processes in the foreground group
  while((pid = waitpid(-1*job,&status,WUNTRACED))!=-1);
  if(!job_status(jman,job,status,FALSE))
    return FALSE;

  return TRUE;
}

/**
* looks for job status changes
* pman: ptr to job manager
* returns: status
**/
_BOOL job_reap(JMANAGER *jman){
  int status; //status returned by waitpid
  pid_t pid; // pid returned by waitpid
  pid_t pgid; //pgid to wait on
  int job =1;
  //loop though all jobs
  for(;job<=MAX_JOBS; job++){
    int tmp_status; // status returned by waitpid
    pid_t tmp_pid =-1; //pid returned by waitpid
    //if a job is active
    if((pgid=jman->jobpgrids[job-1])!=-1){
      errno = 0;
      //check if any procs in that job have changed status
      while((tmp_pid = waitpid(-1*pgid,&tmp_status,WNOHANG | WUNTRACED | WCONTINUED)) !=0 && tmp_pid!=-1){
        status = tmp_status; //store the last status
        pid = tmp_pid; //this is needed since all procs have been stopped -1 will never be returned
                      //this checks if a proc was returned at some point
      }
      if(errno == ECHILD || pid != -1){ //no more procs left or proceses were stopped or continued
        job_status(jman,job,status,TRUE);
      }
      else if(errno !=ECHILD && errno != 0 && tmp_pid==-1)
        return FALSE; //an error
    }
  }

  return TRUE;
}



/**
* parses processes wait status when it is cleaned up
* pman: process manager
* job: pid of job that completed
* status: status from waiting on that job
* done_print: whether to print (DONE) for finished process
* returns: status
**/
_BOOL job_status(JMANAGER *jman,int job, int status,_BOOL ground){
  errno = 0;

  //job failed to be started with some error(print it)
  if(jman->err[job-1]!=0){
    errno = jman->err[job-1];
    perror(jman->jobnames[index]);
    errno = 0;
    if(!job_destroy(jman,job))
      return FALSE;
    return TRUE;
  }
  //if job was suspended
  if(WIFSTOPPED(status)){
    //if it was the cause of ctl-Z
    jman->suspendedstatus[job-1]=TRUE;
    printf("[%d]  Stopped                   %s",job,jman->jobnames[job-1]);
    if(!ground)
      jman->recent_foreground_status = WSTOPSIG(status);
  }
  //if process was killed by a signal and backgrounded
  else if(WIFSIGNALED(status) && ground){

    //if it was SIGKILL, print the killed msg
    char *str_sig;
    if((str_sig=strsignal(WTERMSIG(status))) == NULL)
      return FALSE;
    char *str_core = "";
    #ifdef WCOREDUMP
    if(WCOREDUMP(status))
      str_core = "(core dumped)";
    #endif
    printf("[%d]  %s %s                  %s",job,str_sig,str_core,jman->jobnames[job-1]);
    //convert signal to strmsg

    //clean up process entry
    if(!job_destroy(jman,job))
      return FALSE;

  }

  else if(WIFSIGNALED(status)){
    char *str_sig;
    if((str_sig=strsignal(WTERMSIG(status))) == NULL)
      return FALSE;
    char *str_core = "";
    #ifdef WCOREDUMP
    if(WCOREDUMP(status))
      str_core = "(core dumped)";
    #endif
    printf("%s %s",str_sig,str_core);
    //convert signal to strmsg
    jman->recent_foreground_status = WTERMSIG(status);
    //clean up process entry
    if(!job_destroy(jman,job))
      return FALSE;
  }

  //process just exited
  else if(WIFEXITED(status) && ground){
    printf("[%d] Done                     %s",job,pman->processnames[index]);
    //clean up process entry
    if(!job_destroy(jman,job))
      return FALSE;
  }
  else if(WIFEXITED(status)){
    jman->recent_foreground_status = WEXITSTATUS(status);
    if(!job_destroy(jman,job))
      return FALSE;
  }


  printf("\n");



}

/**
* cleans up job
* jman: job manager
* job: job to clean up
**/
_BOOL job_destroy(JMANAGER *jman, int job){
  memset(jman->jobsnames[job-1],0,MAX_JOB_NAME);
  jman->jobpgrids[job-1] = -1;
  jman->suspendedstatus[job-1] = FALSE;
  jman->err[job-1] = 0;

}


/**
* moves job ground
* pman: ptr to process manager
* job: process id of job to move to foreground
* returns: status
**/
_BOOL job_ground_change(JMANAGER *jman,int job,_BOOL ground){
  if(jman->suspendedstatus[job-1]!= TRUE){
    errno = EINVAL;
    return FALSE
  }

  if(kill(pman->jobpgrids[job-1],SIGCONT) == -1)
    return FALSE;
  jman->suspendedstatus[job-1] = TRUE;

  if(!ground){
    if(!job_wait_foreground(jman,job))
      return FALSE;
  }
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
  if(num_embryos <=0){
    errno = EINVAL;
    return FALSE;
  }

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
  int fork_seq = embryos[index].fork_seq; //current set of forked processes
  _BOOL end = FALSE;
  while(!end){
    _BOOL set = FALSE;
    while(index<num_embryos && embryos[index].fork_seq == fork_seq){
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
          if(!set){
            strcpy(jman->jobnames[fork_seq-1],info->forkseqname[fork_seq-1]);
            jman->jobpgrids[fork_seq-1] = pid;
            setpgid(pid,pid);
            set = TRUE;
          }
          else{
            setpgid(pid,jman->jobpgrids[fork_seq-1]);
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
              jman->err[fork_seq-1] = err;
              //notify caller of error
              err_ind = fork_seq; //a proc in the current fork seq failed
              break;
            }
          }
          break;
        }
      }

      index++;
    }

    if(!info->background[fork_seq-1]){
      if(!process_wait_foreground(jman,fork_seq))
        return FALSE;
    }

    //if there are more embryos
    if(index<num_embryos){
        //if not wait for the current foreground group
        //whether the proc is background or not doesn't matter since having a & before an && is invalid syntax
        if(err_ind != 0) // the last fork_seq needs to succeed to move on
          return FALSE; //if this is true we can't continue
        //update for_seq for next time
        fork_seq = embryos[index].fork_seq;
    }
    else{
      end = TRUE;
    }

  }

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
