#ifndef ID_H
#define ID_H
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include "../../bool.h"
#define NOT_APPL (long)-1
#define NOMEMBUF(buf, str)  (strlen(str) + 1 > buf) ? TRUE: FALSE

typedef struct PROCESS_USER{

  uid_t rid; //real uid
  uid_t eid; //eff uid
  uid_t suid; // saved-set uid


  gid_t rgid;  //real gid
  gid_t egid; //eff gid
  gid_t sgid;  // saved-set gid

  gid_t grouplist[NGROUPS_MAX+1]; //supp gids
  int num_grps;


}USER;

#define CONTX  1
#define GROUP  2
#define GROUPS 4
#define NAME   8
#define REAL   16
#define USR    32
#define HELP   64
#define VERS   128


//creates a bit mask for options
typedef union _ID_OPTIONS{

  struct {
    unsigned char Z:1;
    unsigned char g:1;
    unsigned char G:1;
    unsigned char n:1;
    unsigned char r:1;
    unsigned char u:1;
    unsigned char h:1;
    unsigned char v:1;
  } bits;
  char byte;

} ID_OPTIONS;



#endif
