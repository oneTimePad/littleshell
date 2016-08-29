



_BOOL shell_pipe(EMBRYO *embryos,EMBRYO_INFO *info){
  int pipes[2];
  //invalid if : there is no current process, a pipe is already present, or the current process is backgrounded( i.e proc1 & | proc2 is invalid)
  if(info->cur_proc == -1 || info->continuing|| procs[info->cur_proc].p_stdout!=-1 || info->last_sequence == PIPE || *procs[info->cur_proc].background){
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

_BOOL shell_redirstdin(EMBRYO *embryos, EMBRYO_INFO *info){

}

_BOOL shell_redirstdout(EMBRYO *embryos, EMBRYO_INFO *info,char which){

}


_BOOL shell_and(EMBRYO *embryos, EMBRYO_INFO *info){

}

_BOOL shell_background(EMBRYO *embryos, EMBRYO_INFO *info){
  
}
