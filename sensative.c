



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

}



_BOOL post_redirstdin_handler(EMBRYO *embryos, EMBRYO_INFO *info, EMBRYO *new_proc){

}



_BOOL post_redirstdout_handler(EMBRYO *embryos, EMBRYO_INFO *info, EMBRYO *new_proc){

}
