#ifndef GETFACL_H
#define GETFACL_H
#include "../../bool.h"
#include "../../errors.h"

#define ACL_OK 0
#define NO_MORE_ENTRIES 0
#define NOMEMBUF(buf, str)  (strlen(str) + 1 > buf) ? TRUE: FALSE

#endif
