



_BOOL pre_pipe_handler(EMBRYO *embryos,EMBRYO_INFO *info){
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

_BOOL post_pipe_handler(EMBRYO *embryos,EMBRYO_INFO *info){

}

_BOOL pre_redirstdin_handler(EMBRYO *embryos, EMBRYO_INFO *info){

}

_BOOL post_redirstdin_handler(EMBRYO *embryos, EMBRYO_INFO *info){

}



_BOOL pre_redirstdout_handler(EMBRYO *embryos, EMBRYO_INFO *info,char which){

}

_BOOL post_redirstdout_handler(EMBRYO *embryos, EMBRYO_INFO *info){

}

_BOOL pre_and_handler(EMBRYO *embryos, EMBRYO_INFO *info){

}

_BOOL pre_background_handler(EMBRYO *embryos, EMBRYO_INFO *info){

}
