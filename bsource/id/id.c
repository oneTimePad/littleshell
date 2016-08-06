
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <getopt.h>
#include "../../errors.h"
#include "id.h"



/**
* converts gid into corresponding name
* gid: id to converts
* name: buffer with enough space to hold grp name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
_BOOL getnamefromgid(gid_t gid, char *name, size_t buf_size){

  struct group * grp;
  errno = 0;
  if((grp = getgrgid(gid))==NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(buf_size,grp->gr_name)){
    errno = ENOMEM;
    return FALSE;
  }

  strncpy(name,grp->gr_name,buf_size);
  return TRUE;

}

/**
* converts uid into corresponding name
* uid: id to converts
* name: buffer with enough space to hold usr name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
_BOOL getnamefromuid(uid_t uid, char *name, size_t buf_size){
  struct passwd* pw;
  errno = 0;
  if((pw = getpwuid(uid)) == NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(buf_size,pw->pw_name)){
    errno = ENOMEM;
    return FALSE;
  }

  strncpy(name,pw->pw_name,buf_size);
  return TRUE;
}

/**
* retrieve user id information
* returns: static user struct
**/
USER * getuserinfo(void){
  static USER useri;
  memset(&useri,0,sizeof(USER));
  #ifdef GNU_SOURCE
  errno = 0;
  if(getresuid(&useri.rid,&useri.eid,&useri.suid) == -1)
      return NULL; //errno is set, return null

  if(getresgid(&useri.rgid,&useri.egid,&useri.sgid) == -1)
      return NULL;
  #else

  useri.rid = getuid();
  useri.eid = geteuid();
  useri.suid = NOT_APPL;

  useri.rgid = getgid();
  useri.egid = getegid();
  useri.sgid = NOT_APPL;

  #endif
  errno = 0;
  if(getgroups(NGROUPS_MAX+1,useri.grouplist) == -1)
    return NULL;

  return &useri;
}

struct option long_options[] = {
  {"context", no_argument,    0 , 'Z'},
  {"group"  , no_argument,    0 , 'g'},
  {"groups" , no_argument,    0 , 'G'},
  {"name"   , no_argument,    0 , 'n'},
  {"real"   , no_argument,    0 , 'r'},
  {"user"   , no_argument,    0 , 'u'},
  {"help"   , no_argument,    0 , 'h'},
  {"version", no_argument,    0 , 'v'},
  { 0,             0,         0,   0 }
};


int
main(int argc, char* argv[]){


  ID_OPTIONS opt_mask;

  int opt =0;
  int long_index = 0;
  while((opt = getopt_long(argc,argv,"ZgGnruhv", long_options, &long_index)) != -1){
      switch(opt){
          case 'Z':
            opt_mask.bits.Z =1; //security context
            break;
          case 'g':
            opt_mask.bits.g =1; //print group
            break;
          case 'G':
            opt_mask.bits.G =1; //print groups
            break;
          case 'n':
            opt_mask.bits.n =1; //print name
            break;
          case 'r':
            opt_mask.bits.r =1; //print rid
            break;
          case 'u':
            opt_mask.bits.u =1; //print eff uid only
            break;
          case 'h':
            opt_mask.bits.h =1; //print help
            break;
          case 'v':
            opt_mask.bits.v =1; //print verbose
            break;
          case '?':
            errExit("%s%s\n","unkown option ",opterr);
            break;
          default:
            errExit("%s\n","error occured while parsing options");


      }
  }

  USER* useri = getuserinfo();

  char name_buf[LOGIN_NAME_MAX];
  if(!getnamefromuid(useri->rid,name_buf,LOGIN_NAME_MAX))
    errnoExit("getnamefromuid()");
  printf("uid=%d(%s) ",useri->rid,name_buf);

  exit(EXIT_SUCCESS);
}
