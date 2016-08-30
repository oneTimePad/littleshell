



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

  //syntax error near  unexpected token ' '
  if(info->cur_proc == -1 || info->last_sequence != '\0' || *procs[info->cur_proc].background){
    errno = EINVAL;
    info->err_character = which;
    return FALSE;
  }
  if(!add_to_job_name(info,which))
    return FALSE;
  //bash chooses to ignore the pipe if stdout is redirected already, this shell follows that
  if(procs[info->cur_proc].p_stdout!=-1){
    info->last_sequence = PIPE;
    return TRUE;
  }

  if(pipe(pipes) == -1)
    return FALSE;
  //start pipeline
  procs[info->cur_proc].p_stdout = pipes[1];
  procs[info->cur_proc].p_pipe_read = pipes[0];
  info->last_sequence = PIPE;
  return TRUE;
}


_BOOL pre_redirio_handler(EMBRYO *embryos, EMBRYO_INFO *info,char which){
  //syntax error near  unexpected token ' '
  if(info->cur_proc == -1 || info->last_sequence != '\0'){
    errno = EINVAL;
    info->err_character = which;
    return FALSE;
  }
  if(!add_to_job_name(info,which))
    return FALSE;

  info->last_sequence = which;
  return TRUE;
}

_BOOL pre_ampersan_handler(EMBRYO *embryos, EMBRYO_INFO *info, char which){
  //syntax error near  unexpected token ' '
  if(info->cur_proc == -1 || info->last_sequence != '\0'){
    errno = EINVAL;
    info->err_character = which;
    return FALSE;
  }
  if(!add_to_job_name(info,which))
    return FALSE;
  //set all processes in pipe to background
  if(which == ANDIN)
    *procs[info->cur_proc].background = TRUE;
  info->fork_seq++;
  info->last_sequence = which;
  return TRUE;
}




_BOOL post_pipe_handler(EMBRYO *embryos,EMBRYO_INFO *info, EMBRYO *new_proc){
  if(!embryo_create(embryos,info))
    return FALSE;
  if(embryos[info->cur_proc-1].p_pipe_read == -1){
    info->last_sequence = '\0';
    return TRUE;
  }
  embryos[info->cur_proc].background = embryos[info->cur_proc-1].background;
  embryos[info->cur_proc].p_stdin = embryos[info->cur_proc-1].p_pipe_read;
  return TRUE;
}



_BOOL post_redirio_handler(EMBRYO *embryos, EMBRYO_INFO *info){


}



_BOOL post_redirstdout_handler(EMBRYO *embryos, EMBRYO_INFO *info, EMBRYO *new_proc){

}
