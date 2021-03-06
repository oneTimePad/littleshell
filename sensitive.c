#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "tokenizer.h"
#include "path.h"
#include "sensitive.h"




// pre- handlers, executed when a specific set of characters are specified in shell input

/**
* handles '|' when it is spotted, deals with first-prog in 'first-prog | second-prog'
* procs: list of embryos
* info: current embryo_init context
* which: the '|' character, used for portability among other handlers that need it
* returns: status
**/
_BOOL pre_pipe_handler(EMBRYO *procs,EMBRYO_INFO *info,char which){
  int pipes[2];

  //syntax error near  unexpected token ' '
  if(info->cur_proc == -1 || info->last_sequence != '\0' || info->background[info->fork_seq]){
    errno = EINVAL;
    return FALSE;
  }
  if(!add_to_job_name(info,which))
    return FALSE;
  //bash chooses to ignore the pipe if stdout is redirected already, this shell follows that
  if(procs[info->cur_proc].p_stdout!=-1){
    info->last_sequence = which;
    return TRUE;
  }

  if(pipe(pipes) == -1)
    return FALSE;
  //start pipeline
  procs[info->cur_proc].p_stdout = pipes[1];
  procs[info->cur_proc].p_pipe_read = pipes[0];
  info->last_sequence = which;
  return TRUE;
}

/**
* handles i/o redirection characters when they occur
* embryos: list of embryos
* info: current context of embryos_init
* which: the i/o redir character
* returns: status
**/
_BOOL pre_redirio_handler(EMBRYO *embryos, EMBRYO_INFO *info,char which){
  //syntax error near  unexpected token ' '
  if(info->cur_proc == -1 || info->last_sequence != '\0'){
    errno = EINVAL;
    return FALSE;
  }
  if(!add_to_job_name(info,which))
    return FALSE;

  info->last_sequence = which;
  return TRUE;
}

/**
* handlers ampersan characters i.e '&' and '&&' when they occur
* embryos: list of embryos
* info: current context of embryos_init
* which: the ampersan character & or &&
* returns: status
**/
_BOOL pre_ampersan_handler(EMBRYO *embryos, EMBRYO_INFO *info, char which){
  //syntax error near  unexpected token ' '
  if(info->cur_proc == -1 || info->last_sequence != '\0'){
    errno = EINVAL;
    return FALSE;
  }
  if(!add_to_job_name(info,which))
    return FALSE;
  //set all processes in pipe to background
  if(which == BACK_GR)
    info->background[info->fork_seq-1] = TRUE;
 
  info->fork_seq++;
  info->last_sequence = '\0';
  return TRUE;
}



//post-handlers, executed when extra input is read in the from the shell and info->last_sequence specified a certain character

/**
* handlers the other end of the pipe, writes the read end to new proc
* embryos: lsit of embryos
* info: current context of embryos_init
* name: next input after a certain last_sequence was seen
* returns: status
**/
_BOOL post_pipe_handler(EMBRYO *embryos,EMBRYO_INFO *info, char *name){
  //create a new embryo
  if(!embryo_create(embryos,info,name))
    return FALSE;
  //if i/o redir occured and then there is a pipe bash ignores the pipe, this shell does too
  if(embryos[info->cur_proc-1].p_pipe_read == -1){
    info->last_sequence = '\0';
    return TRUE;
  }
  //change stdin to pipe read
  embryos[info->cur_proc].p_stdin = embryos[info->cur_proc-1].p_pipe_read;
  info->last_sequence = '\0';
  return TRUE;
}


/**
* actually opens the file for redirecting i/o, changes the current embryos stdio
* embryos: list of embryos
* info: current context of embryos_init
* name: the current token value
* returns: status
**/
_BOOL post_redirio_handler(EMBRYO *embryos, EMBRYO_INFO *info, char *name){
  errno = 0;
  int fd;
  if((fd = open(name, (info->last_sequence==RDR_SIN)? O_RDONLY :(info->last_sequence==RDR_SOT_A) ? O_APPEND | O_WRONLY : O_WRONLY) ) == -1){
	if(errno = ENOENT && info->last_sequence!= RDR_SIN){
		errno = 0;
		fd = open(name,O_CREAT | O_EXCL | O_WRONLY,S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH); //rw-r-r
		if(fd == -1){
			return FALSE;
		}
	}
	else{
		return FALSE;
	}	


  }


  if(info->last_sequence == RDR_SIN)
    embryos[info->cur_proc].p_stdin = fd;
  else
    embryos[info->cur_proc].p_stdout = fd;

  info->last_sequence = '\0';
  return TRUE;

}

/**
* just added for consistancy, just creates a new proc and resets last_sequence
* ampersan doesn't actually have any post action
* embryos: list of embryos
* info: current context of embryos_init
* name: current token value
**/
_BOOL post_ampersan_handler(EMBRYO *embryos, EMBRYO_INFO *info, char *name){
  if(!embryo_create(embryos,info,name))
    return FALSE;
  info->last_sequence = '\0';
  return TRUE;
}

//contains pre and post handlers for embryos_init

prehandler prehandlers[NUM_PRE_HANDLERS] ={
  NULL,
  pre_pipe_handler,
  pre_redirio_handler,
  pre_redirio_handler,
  pre_redirio_handler,
  pre_ampersan_handler,
  pre_ampersan_handler,
  NULL
};

posthandler posthandlers[NUM_POST_HANDLER] = {
  embryo_arg,
  post_pipe_handler,
  post_redirio_handler,
  post_redirio_handler,
  post_redirio_handler,
  post_ampersan_handler,
  post_ampersan_handler,
  NULL
};
