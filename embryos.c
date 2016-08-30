










/**
* determines if first char in str is a shell understood character
* returns: TRUE if is, FALSE if not
**/
static char* sensative_characters[] ={
  "","|",">",">>","<","&","&&",NULL
};
int isSensitive(char *str){
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


_BOOL add_to_job_name(EMBRYO_INFO *info,char which){
  char *sens_char = sensative_characters[isSensitive(&which)];
  if(strlen(info->forkseqname[info->fork_seq-1])+strlen(sens_char)+2 >MAX_JOB_NAME){
    errno = ENOMEM;
    return FALSE;
  }
  strcat(info->forkseqname[info->fork_seq-1]," ");
  strcat(info->forkseqname[info->fork_seq-1],sens_char);
  return TRUE;
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


/**
* adds a new embryo to the list of embryos
* procs: list of embryos
* info: information about current execution context of embryos_init
* name: name of this embryo
**/
_BOOL embryo_create(EMBRYO *procs,EMBRYO_INFO *info, char *name){
  if(info->cur_proc+1 >= size){
    errno = ENOMEM;
    return FALSE;
  }

  EMBRYO * new_proc = &procs[++info->cur_proc]; //retrieve a new proc entry
  new_proc->fork_seq = info->fork_seq; //set the fork sequence

  //starts forming the job name
  //first entry in the job
  //forkseqname is the pre-cursor to jobname
  if(info_cur_proc-1 <0 || procs[info->cur_proc-1].fork_seq != new_proc->fork_seq){
    if(strlen(name)+1 > MAX_JOB_NAME){
      errno = ENOMEM;
      return FALSE;
    }
    strcpy(info->forkseqname[info->fork_seq-1],name);
  }
  //this is not the first entry in the job
  else{
    if(strlen(name)+strlen(info->forkseqname[info->fork_seq-1])+2 > MAX_JOB_NAME){
      errno = ENOMEM;
      return FALSE;
    }
    strcat(info->forkseqname[info->forkseqname-1]," ");
    strcat(info->forkseqname[info->forkseqname-1],name);

  }

  //form the acutall path name for this embryo( i.e. what goes into execve)
  new_proc->internal_command = FALSE;
  //attempt to get the process name and check if it is in the path if necessary
  if(strlen(name)+1>PATH_LIM){
    info->cur_proc-=1;
    errno = ENOMEM;
    return FALSE;
  }
  short key = NONE;
  new_proc->internal_key = NONE;
  if(((strstr(name,"/")!= NULL) ? strcpy(new_proc->program,name) :( (inPath(cur_tkn,new_proc->program,PATH_LIM)&& (key=inInternal(name))==NONE) ? new_proc->program : NULL) ) ==NULL){
    //it might be a command internal to the shell
    if((key!=NONE){
      new_proc->internal_key = key;
    }
    else{
        info->cur_proc-=1;
        errno = ENOENT;
        return FALSE;
    }
  }
  //set up args
  new_proc->num_args = 0;
  //set up file-descriptors
  new_proc->my_pipe_other = -1;
  new_proc->p_stdin = -1;
  new_proc->p_stdout = -1;

}

/**
* add arguments to the current embryo, this is a special posthandler
*procs: list of embryos
* info: current context of embryos_init
* arg: the argument to add
**/
_BOOL embryo_arg(EMBRYO *procs,EMBRYO_INFO *info, char *arg){
  //check for overflow
  if(embryos[info->cur_proc].num_args >= MAX_ARGUMENT || strlen(arg) + 1 > MAX_ARG_LEN){
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

    //replace these with function pointers
    //for prehandlers
    //certain handlers can choose to get the current process
    //or create a new owner
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
          if(info->cur_proclinux SIGTTO == -1|| info->continuing || info->pipe_present){
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
        //put post handlers here, if there is no post handler
        //just reset and create new proc
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
