
#ifndef _EXEC
#define _EXEC
#include "bool.h"
#include "tokenizer.h"
#include "processmanager.h"

#ifndef MAX_EMBRYOS
  #define MAX_EMBRYOS 256
#endif




_BOOL execute(PMANAGER *, TOKENS *,EMBRYO *,EMBRYO_INFO *);
#endif
