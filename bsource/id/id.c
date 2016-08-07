
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <getopt.h>
#include "../../errors.h"
#include "id.h"




/**
* determines whether the option mask is valid
* (i.e. can't have multiple "only" options
* mask: ptr to bitmask options
* returns: status
**/
static inline _BOOL validateMask(const ID_OPTIONS* mask){

  int count =0;

  count+= ((mask->byte&GROUP) ? 1 : 0);
  count+= ((mask->byte&CONTX) ? 1 : 0);
  count+= ((mask->byte&USR)   ? 1 : 0);

  int others=0;

  others+=((mask->byte&GROUPS)? 1 : 0);
  others+=((mask->byte&NAME)  ? 1 : 0);
  others+=((mask->byte&REAL)  ? 1 : 0);
  others+=((mask->byte&HELP)  ? 1 : 0);
  others+=((mask->byte&VERS)  ? 1 : 0);

  return ( (count!=1 && count!=0) || (count!=0 && others!=0)) ? FALSE : TRUE;

}


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

  strncpy(name,pw->pw_name,buf_size);
  return TRUE;
}

/**
* retrieve user id information
* returns: static user struct
**/
static USER * getuserinfo(const char* optional_user){


  static USER useri;
  memset(&useri,0,sizeof(USER));

  if(optional_user==NULL){

    //if linux, we can use easy fct's
    #ifdef GNU_SOURCE
    errno = 0;
    if(getresuid(&useri.rid,&useri.eid,&useri.suid) == -1)
        return NULL; //errno is set, return null

    if(getresgid(&useri.rgid,&useri.egid,&useri.sgid) == -1)
        return NULL;
    //else the long way
    #else

    useri.rid = getuid();
    useri.eid = geteuid();
    useri.suid = NOT_APPL; //no way to get this

    useri.rgid = getgid();
    useri.egid = getegid();
    useri.sgid = NOT_APPL;

    #endif
    errno = 0;
    int supp_grp_num;
    if((supp_grp_num=getgroups(NGROUPS_MAX+1,useri.grouplist)) == -1)
      return NULL;

    useri.num_grps = supp_grp_num;
  }
  else{
      struct passwd* pw;
      errno = 0;
      if((pw=getpwnam(optional_user)) == NULL){
        errnoExit("getpwnam()");
        errExit("%s \"%s\" %s\n","user",optional_user,"notfound");
      }
      useri.rid = pw->pw_uid;
      useri.rgid = pw->pw_gid;

      useri.eid = NOT_APPL;
      useri.egid = NOT_APPL;

      useri.suid = NOT_APPL;
      useri.sgid = NOT_APPL;

      //portable way of determing if user is part of group
      int grp_id =-1;
      errno = 0;
      struct group* grp_entry;
      while((grp_entry=getgrent())!=NULL){

        char** grp_mem_list = grp_entry->gr_mem;
        //if it owns the group
        if(strncmp(optional_user,grp_entry->gr_name,strlen(grp_entry->gr_name))==0){
          useri.grouplist[++grp_id] = grp_entry->gr_gid;
          break;
        }
        //check if it is in that group
        for(;*grp_mem_list!=NULL;grp_mem_list++){
          if(strncmp(optional_user,*grp_mem_list,strlen(*grp_mem_list))==0){
            useri.grouplist[++grp_id] = grp_entry->gr_gid;
            break;
          }
        }
      }
      endgrent();
      errnoExit("getgrent()"); //checks if errno is not 0
      if(grp_id!=-1)
        useri.num_grps = grp_id+1;

  }

  return &useri;
}

static struct option long_options[] = {
  {"context", no_argument,    0 , 'Z'},
  {"group"  , no_argument,    0 , 'g'},
  {"groups" , no_argument,    0 , 'G'},
  {"name"   , no_argument,    0 , 'n'},
  {"real"   , no_argument,    0 , 'r'},
  {"user"   , no_argument,    0 , 'u'},
  {"help"   , no_argument,    0 , 'h'},
  {"verbose", no_argument,    0 , 'v'},
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



  if(!validateMask(&opt_mask))
    errExit("%s\n", "id cannot print \"only\" of more than one choice");

  char *user = (argc>1) ? argv[((optind>0)? optind : optind+1)] : NULL;
  USER* useri = getuserinfo(user);

  if(opt_mask.byte&GROUP){
    (user==NULL)? printf("%d\n",useri->egid) : errExit("%s\n","invalid option when user is specifed");
    fflush(stdout);
    exit(EXIT_SUCCESS);
  }

  if(opt_mask.byte&USR){
    (user==NULL)? printf("%d\n",useri->eid) : errExit("%s\n","invalid option when user is specifed");
    fflush(stdout);
    exit(EXIT_SUCCESS);
  }

  if(opt_mask.byte&GROUPS){
    int max_grp_id =useri->num_grps, grp_id = 0;
    for(;grp_id<max_grp_id;grp_id++)
      printf("%d ",useri->grouplist[grp_id]);
    printf("\n");
    fflush(stdout);
    exit(EXIT_SUCCESS);
  }

  char name_buf[LOGIN_NAME_MAX];
  if(!getnamefromuid(useri->rid,name_buf,LOGIN_NAME_MAX))
    errnoExit("getnamefromuid()");
  printf("uid=%d(%s) ",useri->rid,name_buf);

  if(!getnamefromgid(useri->rgid,name_buf,LOGIN_NAME_MAX))
    errnoExit("getnamefromgid()");
  printf("gid=%d(%s) ",useri->rgid,name_buf);

  printf("groups=%d(%s)",useri->rgid,name_buf);

  int max_grp_id =useri->num_grps, grp_id = 1;
  for(;grp_id <max_grp_id-1; grp_id++ ){
    if(!getnamefromgid(useri->grouplist[grp_id],name_buf,LOGIN_NAME_MAX))
      errnoExit("getnamefromgid()");
    printf(",%d(%s)",useri->grouplist[grp_id],name_buf);
  }
  fflush(stdout);
  printf("\n");


  if(opt_mask.byte&VERS){
    if(useri->eid!=-1){
      if(!getnamefromuid(useri->eid,name_buf,LOGIN_NAME_MAX))
        errnoExit("getnamefromuid()");
      printf("euid=%d(%s) ",useri->eid,name_buf);
    }

    if(useri->suid!=-1){
      if(!getnamefromuid(useri->suid,name_buf,LOGIN_NAME_MAX))
        errnoExit("getnamefromuid()");
      printf("suid=%d(%s) ",useri->suid,name_buf);
    }
    if(useri->egid!=-1){
      if(!getnamefromgid(useri->egid,name_buf,LOGIN_NAME_MAX))
        errnoExit("getnamefromgid()");
      printf("egid=%d(%s) ",useri->egid,name_buf);
    }

    if(useri->sgid!=-1){
      if(!getnamefromgid(useri->sgid,name_buf,LOGIN_NAME_MAX))
        errnoExit("getnamefromgid()");
      printf("sgid=%d(%s) ",useri->sgid,name_buf);
    }
    fflush(stdout);
    printf("\n");

  }

  exit(EXIT_SUCCESS);
}
