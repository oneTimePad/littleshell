#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/acl.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <libgen.h>
#include "getfacl.h"




/**
* converts gid into corresponding name
* gid: id to converts
* name: buffer with enough space to hold grp name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
static _BOOL getnamefromgid(gid_t gid, char *name, size_t buf_size){

  struct group * grp;
  errno = 0;
  if((grp = getgrgid(gid))==NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(buf_size,grp->gr_name)){
    errno = ENOMEM;
    return FALSE;
  }

  strcpy(name,grp->gr_name);
  return TRUE;

}

/**
* converts uid into corresponding name
* uid: id to converts
* name: buffer with enough space to hold usr name and null-term
* buf_size: size of buf
* returns: int status, sets errno
**/
static _BOOL getnamefromuid(uid_t uid, char *name, size_t buf_size){
  struct passwd* pw;
  errno = 0;
  if((pw = getpwuid(uid)) == NULL)
    return FALSE;
  errno = 0;
  if(NOMEMBUF(buf_size,pw->pw_name)){
    errno = ENOMEM;
    return FALSE;
  }

  strcpy(name,pw->pw_name);
  return TRUE;
}


static _BOOL print_long_entry(const char *type,const char *qual,acl_entry_t *entry){
  acl_permset_t perm_set;
  if(acl_get_permset(*entry,&perm_set)!=ACL_OK)
    return FALSE;
  printf("%s:%s:%c%c%c\n",type,qual,(!acl_get_perm(perm_set,ACL_READ))? '-':'r',
                        (!acl_get_perm(perm_set,ACL_WRITE))? '-' :'w',
                        (!acl_get_perm(perm_set,ACL_EXECUTE))?'-':'x');
  fflush(stdout);

  return TRUE;
}

int
main(int argc, char *argv[]){

  if(argc<2)
      usageExit("%s filename\n",argv[0]);

  char *filename = argv[1];



  struct stat stats;
  if(stat(filename,&stats)==-1)
    errnoExit("stat()");

  printf("# file: %s\n",basename(filename));

  char user[LOGIN_NAME_MAX+1];
  char group[LOGIN_NAME_MAX+1];

  if(!getnamefromuid(stats.st_uid,user,LOGIN_NAME_MAX))
    errnoExit("getnamefromuid()");
  if(!getnamefromgid(stats.st_gid,group,LOGIN_NAME_MAX))
    errnoExit("getnamefromgid()");

  printf("# owner: %s\n",user);
  printf("# group: %s\n",group);

  acl_t acl;

  if((acl=acl_get_file(filename,ACL_TYPE_ACCESS))==(acl_t)NULL)
    errnoExit("acl_get_file()");

    int entry_ind = ACL_FIRST_ENTRY;
    acl_entry_t entry;
    //loop through current entries
    for(;acl_get_entry(acl,entry_ind,&entry)!=NO_MORE_ENTRIES;entry_ind = ACL_NEXT_ENTRY){
      acl_tag_t tag;
      if(acl_get_tag_type(entry,&tag)!=ACL_OK)
        errnoExit("acl_get_tag_type()");

      switch (tag) {
        case ACL_USER_OBJ:{ //modifies the user_obj if told to
          if(!print_long_entry("user","",&entry))
            errnoExit("print_long_entry()");
          break;
        }
        case ACL_GROUP_OBJ:{ //modifiers the group_obj if told to
          if(!print_long_entry("group","",&entry))
            errnoExit("print_long_entry()");
          break;
        }
        case ACL_OTHER:{ //modifies other if told to
          if(!print_long_entry("other","",&entry))
            errnoExit("print_long_entry()");
          break;
        }
        case ACL_MASK:{ //modifiers mask if told to
          if(!print_long_entry("mask","",&entry))
            errnoExit("print_long_entry()");
          break;
        }
        case ACL_USER:{ //modifies user if told to
          uid_t *uid;
          if((uid = acl_get_qualifier(entry))==NULL)
            errnoExit("acl_get_qualifier()");
          char user[LOGIN_NAME_MAX+1];
          if(!getnamefromuid(*uid,user,LOGIN_NAME_MAX))
            errnoExit("getnamefromuid()");
          if(!print_long_entry("user",user,&entry))
            errnoExit("print_long_entry()");
          break;
        }
        case ACL_GROUP:{ //modifiers group if told to
          gid_t *gid;
          if((gid = acl_get_qualifier(entry))==NULL)
            errnoExit("acl_get_qualifier()");
          char group[LOGIN_NAME_MAX+1];
          if(!getnamefromgid(*gid,group,LOGIN_NAME_MAX))
            errnoExit("getnamefromgid()");
          if(!print_long_entry("group",user,&entry))
            errnoExit("print_long_entry()");
          break;
        }
        default:{
          errno = EINVAL;
          errnoExit("main()");
        }
      }
    }
  exit(EXIT_SUCCESS);

}
