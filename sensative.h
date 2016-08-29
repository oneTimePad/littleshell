#ifndef SENSATIVE_H
#define SENSATIVE_H





_BOOL shell_pipe(EMBRYO *,EMBRYO_INFO *);
_BOOL shell_redirstdin(EMBRYO *, EMBRYO_INFO *);
_BOOL shell_redirstdout(EMBRYO *, EMBRYO_INFO *,char );
_BOOL shell_and(EMBRYO *, EMBRYO_INFO *);
_BOOL shell_background(EMBRYO *, EMBRYO_INFO *);

#endif
