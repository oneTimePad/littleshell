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
* determines if first char in str is a shell understood character
* returns: TRUE if is, FALSE if not
**/
static char* internals[] ={
  "","|",">",">>","<","&","&&",NULL
};
static int isSensitive(char *str){
  switch (*str) {
    case PIPE:
      return 1;
      break;
    case RDR_SOT:
      return 2;
      break;
    case RDR_SOT_A:
      return 3;
      break;
    case RDR_SIN:
    case RDR_SIN_A:
      return 4;
      break;
    case BACK_GR:
      return 5;
      break;
    case ANDIN:
      return 6;
      break;
    default:
      return 0;
      break;
  }
}
/*
* cleans up embryos
* procs: list of embryos created
* info: info struct about the embryos created
*/
_BOOL embryo_clean(EMBRYO *procs,EMBRYO_INFO *info){
  if(info->cur_proc == -1)return TRUE;
  if(info->pipe_present && procs[info->cur_proc-1].my_pipe_other!=-1){
    if(close(procs[info->cur_proc].my_pipe_other)==-1)
      return FALSE;
    procs[info->cur_proc].p_stdout=-1;
  }
  int num_procs = info->cur_proc+1;
  int index = 0;

  for(;index < num_procs;index++){
    if(index ==0 || (index!=0 && procs[index-1].fork_seq !=procs[index].fork_seq)){
      free(procs[index].background);
    }
    if(procs[index].p_stdout!=-1 && procs[index].my_pipe_other==-1){
      if(close(procs[index].p_stdout) == -1)
        return FALSE;
    }
    if(procs[index].p_stdin!=-1){
      if(close(procs[index].p_stdin) == -1)
        return FALSE;
    }
  }
  return TRUE;
}


_BOOL embryo_create(EMBRYO *procs,EMBRYO_INFO *info, char *name){
  if(info->cur_proc+1 >= size){errno = ENOMEM; return FALSE;}

  EMBRYO * new_proc = &procs[++info->cur_proc]; //retrieve a new proc entry
  new_proc->fork_seq = info->fork_seq; //set the fork sequence
  new_proc->internal_command = FALSE;
  //attempt to get the process name and check if it is in the path if necessary
  if(strlen(cur_tkn)+1>PATH_LIM){
    info->cur_proc-=1;
    errno = ENOMEM;
    return FALSE;
  }
  if(((strstr(cur_tkn,"/")!= NULL) ? strcpy(new_proc->program,cur_tkn) :( (inPath(cur_tkn,new_proc->program,PATH_LIM)&& inInternal(cur_tkn)==NONE) ? new_proc->program : NULL) ) ==NULL){
    //it might be a command internal to the shell
    short key;
    if((key=inInternal(cur_tkn))!=NONE){
      new_proc->internal_command = TRUE;
      new_proc->internal_key = key;
    }
    else{
        info->cur_proc-=1;
        errno = ENOENT;
        return FALSE;
      }
  }
  new_proc->num_args = 0;
  new_proc->my_pipe_other = -1;
  new_proc->p_stdin = -1;
  new_proc->p_stdout = -1;

}

_BOOL embryo_arg(EMBRYO *procs,EMBRYO_INFO *info, char *arg){
  if(embryos[info->cur_proc].num_args == MAX_ARGUMENT || strlen(arg) + 1 > MAX_ARG_LEN){
    info->cur_proc-=1;
    errno = ENOMEM;
    return FALSE;
  }

  EMBRYO *proc =  procs[info->cur_proc];
  strcpy(proc->arguments[proc->num_args++],arg);
  return TRUE;

}


/**
* attempts to form processes and their i/o connections from tkns
* tkns: ptr to the TOKENS
* procs: ptr to EMBRYO structs
* info: contains information about where to start( i.e to be used if we are continuing from a previous call to embryos_init)
* returns: status, return FALSE and errno is set to EINVAL for bad syntax or 0 if we don't have enough info (i.e go back to shell)
**/
_BOOL embryos_init(TOKENS *tkns,EMBRYO* procs,size_t size, EMBRYO_INFO* info){
  if(info == NULL | tkns == NULL || procs == NULL) {errno = EFAULT; return FALSE;}
  char *cur_tkn;
  int which = CURR_TOKEN;

  while((cur_tkn = getToken(tkns,which))!=NULL){
      switch (*cur_tkn) {
        case PIPE:{
          if(!ePIPE(procs,info))
            return FALSE;
          which = CURR_TOKEN;

          break;
        }
        case RDR_SIN:{
          //pipeline must not be open to stdin of this proc or if rdr to stdin was already done-> invalid (i.e. proc1 | proc2 < file or proc1 < file1 < file2 is invalid)
          if(info->cur_proc == -1 || info->continuing || procs[info->cur_proc].p_stdin!=-1){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }
          //possibly go back to shell to wait for file
          if((cur_tkn = getToken(tkns,NEXT_TOKEN)) == NULL){info->last_sequence = RDR_SIN;errno = 0; return FALSE;}
          if(isSensitive(cur_tkn)){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }
          int fd;
          if((fd = open(cur_tkn,O_RDONLY))==-1){return FALSE;}
          procs[info->cur_proc].p_stdin = fd;
          which = NEXT_TOKEN;
          procs[info->cur_proc].num_components_job_name++;
          break;
        }
        case RDR_SIN_A:
          break;
        case RDR_SOT:
        case RDR_SOT_A:{
          //pipeline must not be open to stdout of this proc or if rdr to stdout has already be done->invalid (i.e proc1 | > file proc2 or proc1 > file1 > file2 is invalid)
          if(info->cur_proc == -1 || info->continuing || procs[info->cur_proc].p_stdout!=-1){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }

          char *rdr = cur_tkn;
          //possibly go back to shell to wait for file
          if((cur_tkn = getToken(tkns,NEXT_TOKEN)) == NULL){info->last_sequence =*rdr; errno = 0; return FALSE;}
          if(isSensitive(cur_tkn)){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }
          int fd;
          if((fd = open(cur_tkn,O_WRONLY | (RDR_SOT_A == *rdr) ? O_APPEND : 0 ))==-1){return FALSE;}
          procs[info->cur_proc].p_stdout = fd;
          which = NEXT_TOKEN;
          procs[info->cur_proc].num_components_job_name++;
          break;
        }
        case BACK_GR:{
          if(info->cur_proc == -1|| info->continuing || info->pipe_present){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }
          //set all processes in pipe to background
          *procs[info->cur_proc].background = TRUE;
          info->fork_seq++;
          which = NEXT_TOKEN;
          break;
        }
        case ANDIN:{

          if(info->cur_proc == -1 || info->continuing ){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }
          info->fork_seq++;
          if((cur_tkn = getToken(tkns,NEXT_TOKEN)) == NULL){info->last_sequence = ANDIN; errno =0; return FALSE;}
          //increase the fork sequence(i.e next process will be startd in a separate fork sequence)
          if(isSensitive(cur_tkn)){
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;
          }


          which = CURR_TOKEN;
          break;
        }
        default:{ //create the process embryo entry
          if(info->cur_proc+1 >= size){errno = ENOMEM; return FALSE;}
          if(info->cur_proc-1>=0){procs[info->cur_proc].num_components_job_name++;}
          EMBRYO * new_proc = &procs[++info->cur_proc]; //retrieve a new proc entry
          new_proc->start_job_name = cur_tkn;
          new_proc->null-terminated = 0;
          new_proc->fork_seq = info->fork_seq; //set the fork sequence
          new_proc->internal_command = FALSE;
          //attempt to get the process name and check if it is in the path if necessary
          if(strlen(cur_tkn)+1>PATH_LIM){
            info->cur_proc-=1;
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = ENOMEM;
            return FALSE;
          }
          if(((strstr(cur_tkn,"/")!= NULL) ? strcpy(new_proc->program,cur_tkn) :( (inPath(cur_tkn,new_proc->program,PATH_LIM)&& inInternal(cur_tkn)==NONE) ? new_proc->program : NULL) ) ==NULL){
            //it might be a command internal to the shell
            short key;
            if((key=inInternal(cur_tkn))!=NONE){
              new_proc->internal_command = TRUE;
              new_proc->internal_key = key;
            }
            else{
                info->cur_proc-=1;
                if(!embryo_clean(procs,info))
                  return FALSE;
                errno = ENOENT;
                return FALSE;
              }
          }
          int args_cnt = 0;
          //search for aguments for new process
          _BOOL end = FALSE;
          char * args = new_proc->arguments;
          which = NEXT_TOKEN;
          new_proc->num_args = 0;
          while(!end &&(cur_tkn = getToken(tkns,which))!= NULL){
            if (isSensitive(cur_tkn)) { //end search
              end = TRUE;
              which = CURR_TOKEN;
            }  //process args
            else{
                if(args_cnt < MAX_ARGUMENT && strlen(cur_tkn)+1<MAX_ARG_LEN){strcpy(args,cur_tkn);}
                else{
                  info->cur_proc-=1;
                  if(!embryo_clean(procs,info))
                    return FALSE;
                  errno = ENOMEM;
                  return FALSE;
                }
                args = args + strlen(args)+1;
                new_proc->num_args++;
                break;
            }
          }
          new_proc->my_pipe_other = -1;
          new_proc->p_stdin = -1;
          //if a pipe is present and the current proc's fork seq matches the fork seq of the previous proc in the pipe
          if(info->pipe_present && new_proc->fork_seq == procs[info->cur_proc-1].fork_seq){
            new_proc->p_stdin = procs[info->cur_proc-1].my_pipe_other;
            //the last process in pipe which determine the background--ptr them all to the same _BOOL
            new_proc->background = procs[info->cur_proc-1].background;
            info->pipe_present = FALSE;
          }
          //fork seq's don't match but pipe is present( i.e  proc1 && | proc2 occured which is invalid)
          else if(info->pipe_present){
            info->cur_proc-=1;
            if(!embryo_clean(procs,info))
              return FALSE;
            errno = EINVAL;
            return FALSE;

          }
          //else no pipe is present
          else{
            new_proc->background = (_BOOL *)malloc(sizeof(_BOOL));
            *new_proc->background = FALSE;
          }
          new_proc->p_stdout = -1;

          break;
        }
      }
      info->continuing = FALSE;
  }

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
_BOOL jobs_init(JMANAGER *jman,EMBRYO *embryos,size_t num_embryos){
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
    switch (pid) {
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
* initialize process table
* pman: ptr to process manager structure
* returns: status
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


    //set tty for processes
    int my_pid = getpid();
    //put shell in foreground
    tcsetpgrp(0,my_pid);
    pman->foreground_group=my_pid;

    return TRUE;
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
