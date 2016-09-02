#ifndef SENSATIVE_H
#define SENSATIVE_H

#include "bool.h"
#include "embryos.h"


typedef  _BOOL (*prehandler) (EMBRYO *,EMBRYO_INFO *,char);
typedef  _BOOL (*posthandler)(EMBRYO *,EMBRYO_INFO *,char *);






#endif
