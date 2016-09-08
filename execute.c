#include <errno.h>
#include "execute.h"


/**
* pman: process manager
* tkn: tokens
* embryos: list of embryos [fixed size set by MAX_EMBRYOS]
* info: used to keep rentrancy, but maintain a state for continuing function
* returns: status, check info->continuing on failure if errno is 0 and fct returns false
**/
_BOOL execute(JMANAGER *jman, TOKENS *tkns,EMBRYO *embryos,EMBRYO_INFO *info){
  errno = 0;
  //if embryo_init is false and errno is 0, then embry_init is looking for more input
  if(!embryos_init(tkns,embryos,info)){
    return FALSE;
  }

  //reset continuing to remove ambiguity since processes_init returns errno 0 on failure sometimes too
  if(!jobs_init(jman,embryos,info)){
    embryo_clean(embryos,info);
    return FALSE;
  }
  //clean up embryos
  //if(!embryo_clean(embryos,info))
   // return FALSE;

  return TRUE;

}
