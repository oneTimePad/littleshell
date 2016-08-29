#ifndef SENSATIVE_H
#define SENSATIVE_H


typedef  _BOOL (*prehandler) (EMBRYO *,EMBRYO_INFO *,char);
typedef  _BOOL (*posthandler)(EMBRYO *,EMBRYO_INFO *,EMBRYO);

 


_BOOL shell_pipe(EMBRYO *,EMBRYO_INFO *);
_BOOL shell_redirstdin(EMBRYO *, EMBRYO_INFO *);
_BOOL shell_redirstdout(EMBRYO *, EMBRYO_INFO *,char );
_BOOL shell_and(EMBRYO *, EMBRYO_INFO *);
_BOOL shell_background(EMBRYO *, EMBRYO_INFO *);

#endif
