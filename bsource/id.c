
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>



/**
* converts gid into corresponding name
* gid: id to converts
* name: buffer with enough space to hold grp name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
int getnamefromgid(gid_t gid, char *name, size_t buf_size){

  struct group * grp;
  errno = 0;
  if((grp = getgrgid(gid))==NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(name,grp->gr_name)){
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
int getnamefromuid(uid_t uid, char *name, size_t buf_size){
  struct passwd* pw;
  errno = 0;
  if((pw = getpwuid(uid)) == NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(name,pw->pw_name)){
    errno = ENOMEN;
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




int
main(int argc, char* argv[]){




  exit(EXIT_SUCCESS);
}
