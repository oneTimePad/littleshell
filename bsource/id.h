#ifndef ID_H
#define ID_H
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "../../bool.h"
#define NOT_APPL (long)-1
#define NOMEMBUF(buf, str) { (strlen(str) + 1 > buf) ? TRUE: FALSE;}

typedef struct PROCESS_USER{

  uid_t rid; //real uid
  uid_t eid; //eff uid
  uid_t suid; // saved-set uid


  git_t rgid;  //real gid
  gid_t egid; //eff gid
  git_t sgid;  // saved-set gid

  git_t grouplist[NGROUPS_MAX+1]; //supp gids



}USER;

_BOOL getnamefromgid(git_t,char *, size_t);
_BOOL getnamefromuid(uid_t, char *, size_t);
USER * getuserinfo(void);

#endif
