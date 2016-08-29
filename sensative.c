



static prehandler pre_handlers[] ={
  pre_pipe_handler,
  pre_redirstdin_handler,
  pre_redirstdout_handler,
  pre_background_handler,
  pre_and_handler,
  NULL
};

static posthandler post_handlers[] = {
  post_pipe_handler,
  post_redirstdin_handler,
  post_redirstdout_handler,
  NULL
};



_BOOL pre_pipe_handler(EMBRYO *procs,EMBRYO_INFO *info,char which){
  int pipes[2];
  //invalid if : there is no current process, a pipe is already present, or the current process is backgrounded( i.e proc1 & | proc2 is invalid)
  if(info->cur_proc == -1 || info->continuing|| procs[info->cur_proc].p_stdout!=-1 || info->last_sequence != '\0' || *procs[info->cur_proc].background){
    errno = EINVAL;
    return FALSE;
  }
  if(pipe(pipes) == -1)
    return FALSE;
  //start pipeline
  procs[info->cur_proc].p_stdout = pipes[1];
  procs[info->cur_proc].my_pipe_other = pipes[0];
  procs[info->cur_proc].num_components_job_name++;
  info->last_sequence = PIPE;
}

_BOOL pre_redirstdin_handler(EMBRYO *procs, EMBRYO_INFO *info,char which){
  if(info->cur_proc == -1 || info->last_sequence!='\0' || info->continuing || procs[info->cur_proc].p_stdin!=-1){
    errno = EINVAL;
    return FALSE;

  }


  //instead of doing this, just concat to jobnames in
  //embryo
  procs[info->cur_proc].num_components_job_name++;
  info->last_sequence = which;
  return TRUE;
}
_BOOL pre_redirstdout_handler(EMBRYO *embryos, EMBRYO_INFO *info,char which){
  if(info->cur_proc == -1 || info->last_sequence!='\0' || info->continuing || procs[info->cur_proc].p_stdout!=-1){
    errno = EINVAL;
    return FALSE;
  }
  procs[info->cur_proc].num_components_job_name++;
  info->last_sequence = which;
  return TRUE;
}

_BOOL pre_background_handler(EMBRYO *embryos, EMBRYO_INFO *info, char which){
  if(info->cur_proc == -1|| info->continuing || info->last_sequence != '\0'){
    errno = EINVAL;
    return FALSE;
  }
  //set all processes in pipe to background
  *procs[info->cur_proc].background = TRUE;
  procs[info->cur_proc].num_components_job_name++;
  info->fork_seq++;
  info->last_sequence = which;
  return TRUE;
}

_BOOL pre_and_handler(EMBRYO *embryos, EMBRYO_INFO *info, char which){
  if(info->cur_proc == -1 || info->continuing ){
    errno = EINVAL;
    return FALSE;
  }
  info->fork_seq++;
  procs[info->cur_proc].num_components_job_name++;
  info->last_sequence = which;
  return TRUE;
}


_BOOL post_pipe_handler(EMBRYO *embryos,EMBRYO_INFO *info, EMBRYO *new_proc){
  //if a pipe is present and the current proc's fork seq matches the fork seq of the previous proc in the pipe
  if(info->last_sequence == PIPE && new_proc->fork_seq == procs[info->cur_proc-1].fork_seq){
    if(new_proc->p_stdin != -1){
      info->cur_proc-=1;
      errno = EINVAL;
      return FALSE;
    }
    new_proc->p_stdin = procs[info->cur_proc-1].my_pipe_other;
    //the last process in pipe which determine the background--ptr them all to the same _BOOL
    new_proc->background = procs[info->cur_proc-1].background;
    info->last_sequence = '\0';
  }
  //fork seq's don't match but pipe is present( i.e  proc1 && | proc2 occured which is invalid)
  else if(info->last_sequence == PIPE){
    info->cur_proc-=1;
    errno = EINVAL;
    return FALSE;

  }
  //else no pipe is present
  else{
    new_proc->background = (_BOOL *)malloc(sizeof(_BOOL));
    *new_proc->background = FALSE;
  }

  return TRUE;
}



_BOOL post_redirstdin_handler(EMBRYO *embryos, EMBRYO_INFO *info, EMBRYO *new_proc){

}



_BOOL post_redirstdout_handler(EMBRYO *embryos, EMBRYO_INFO *info, EMBRYO *new_proc){

}
