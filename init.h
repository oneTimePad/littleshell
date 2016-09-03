#ifndef INIT_H
#define INIT_H
#include <sys/types.h>
#include "jobmanager.h"

#define LEN_AT 1
#define LEN_COLON 1


typedef struct _INIT{
  void (*term_handler)(int);
  char *path;
  JMANAGER *jman;
  char *line;
  size_t line_size;

} INIT;


_BOOL shell_init(INIT *);

#endif
