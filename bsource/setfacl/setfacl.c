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
#include "setfacl.h"

/**
* converts name into uid equiv
* pw_name : username
* returns: uid or -1 on error
**/
__attribute__((fastcall)) static inline uid_t getuidfromname(const char *pw_name){
  struct passwd* pw;
  if((pw=getpwnam(pw_name))==NULL)
    return (uid_t)-1;
  return pw->pw_uid;
}

/**
* converts name to gid equiv
* gr_name: group name
* retruns: gid or -1 on error
**/
__attribute__((fastcall)) static inline gid_t getgidfromname(const char *gr_name){
  struct group* gr;
  if((gr=getgrnam(gr_name))==NULL)
    return (gid_t)-1;
  return gr->gr_gid;
}

/**
* parsing short-form acl string
* acl_string: acl entries string in short form
* list: list containing acl structs
* num_entries: size of list in terms of number of acl entries
* errno: sets errno to EINVAL if string is malformed, or ENOMEM if no more entries available
*   errno is also set by getuidfromname and getgidfromname
* returns: status and sets errno
**/
_BOOL short_parse_acl(const char* acl_string,size_t string_size, ACLENTRY *list,int num_entries,int *num_used){
  if(num_used==NULL || list == NULL || string_size<=0 ||acl_string==NULL || *acl_string == '\0' || num_entries<=0) return FALSE;
  if(strlen(acl_string)!=string_size) return FALSE;
  //copy over the string
  char acl_string_copy[string_size+1];
  strncpy(acl_string_copy,acl_string,string_size);
  acl_string_copy[string_size]= '\0';
  char * acl_string_cpy = acl_string_copy; //need ptr arithmetic

  volatile int cur_entry=0;
  //used by loop
  volatile _BOOL set_tag = FALSE; //was the tag set?
  volatile _BOOL set_qual = FALSE;//was the qualifier set?
  //loop through short form acl string
  for(;*acl_string_cpy!='\0';acl_string_cpy++){
    if(*acl_string_cpy==','){
      cur_entry++;
      if(cur_entry>=num_entries){
        errno = ENOMEM;
        return FALSE;
      }
      set_tag  = FALSE;
      set_qual = FALSE;
    }
    //if the next char is a ':' and the tag isn't set yet
   else if(*(acl_string_cpy+1) ==':'&&!set_tag){
      //set the tag
      switch (*acl_string_cpy) {
        case 'u': //since we don't know if a qual is specified, we just set to OBJ for now
          (list+cur_entry)->tag = ACL_USER_OBJ;
          (list+cur_entry)->ids.u_qual=-1;
          break;
        case 'g': //same reason for 'u'
          (list+cur_entry)->tag = ACL_GROUP_OBJ;
          (list+cur_entry)->ids.u_qual=-1;
          break;
        case 'o': // other
          (list+cur_entry)->tag = ACL_OTHER;
          (list+cur_entry)->ids.u_qual=-1;
          break;
        case 'm': //mask
          (list+cur_entry)->tag = ACL_MASK;
          (list+cur_entry)->ids.u_qual=-1;
          break;
        default: // it is something unknown
          errno = EINVAL;
          return FALSE;
          break;
      }
      acl_string_cpy++;
      set_tag = TRUE;
    }
    //if the tag is set and the qualifier isn't
    else if(set_tag&&!set_qual){
      //if the char is a ':' that means no qualifier is specified, so just move on
      //this means our guess for tag for USER/GROUP of OBJ was right
      if(*acl_string_cpy == ':'){
        set_qual=TRUE;
        continue;
      }

      //else parse the qualifier
      char *qual_start = acl_string_cpy;
      do
        qual_start++; // keep moving till we find a ':' or '\0'
      while(*qual_start!=':' && *qual_start!='\0');
      if(*qual_start=='\0'){ //the string isn't formatted corectly
        errno = EINVAL;
        return FALSE;
      }
      *qual_start = '\0'; //end the qualifier with a (change ':' to) '\0' (reason necessary is apparent below)

      if((long)((list+cur_entry)->tag) == (long)ACL_USER_OBJ){ //if the tag was user

        uid_t qual; //convert the qualifier name to uid (reason we added '\0')
        if((qual=getuidfromname(acl_string_cpy))==-1){
          if(errno ==0)
            errno = ENOENT;
          return FALSE;
        }
        (list+cur_entry)->ids.u_qual = qual;
        (list+cur_entry)->tag = ACL_USER; //changed tag since qualifier was specified
      }
      else if((long)((list+cur_entry)->tag) == (long)ACL_GROUP_OBJ){ //if the tag was group
        gid_t qual;//convert the qualifier name to gid (reason we added '\0')
        if((qual=getgidfromname(acl_string_cpy))==-1){
          if(errno==0)
            errno = ENOENT;
          return FALSE;
        }
        (list+cur_entry)->ids.g_qual = qual;
        (list+cur_entry)->tag = ACL_GROUP; //changed tag since qualifier was specified
      }
      *qual_start = ':'; //remove the '\0', put it back to ':'
      acl_string_cpy = qual_start;
      set_qual = TRUE;
    }
    //if the tag and qualifier are both set
    else if(set_tag&&set_qual){
      //set the permissions mask (expects 'rwx' order else undefined result)
      (list+cur_entry)->perm.bits.r = (*acl_string_cpy++=='r') ? 1 : 0;
      if(*acl_string_cpy=='\0'){ //make sure the string didn't end for some reason
        errno = EINVAL;
        return FALSE;
      }
      (list+cur_entry)->perm.bits.w = (*acl_string_cpy++=='w')? 1 : 0;
      if(*acl_string_cpy=='\0'){
        errno = EINVAL;
        return FALSE;
      }
      (list+cur_entry)->perm.bits.x = (*acl_string_cpy=='x') ? 1 : 0;

    }
    //else something unexpected was seen in the string
    else{
      errno = EINVAL;
      return FALSE;
    }
  }
  if(set_tag==FALSE || set_qual==FALSE){
    errno = EINVAL;
    return FALSE;
  }
  *num_used = cur_entry+1;
  return TRUE;

}


static struct option long_options[] = {
  {"set", required_argument,    0 , 's'},
  {"modify"  , required_argument,    0 , 'm'},
  {"remove" , required_argument,    0 , 'x'},
  {"set-file"   , required_argument,    0 , 'S'},
  {"modify-file"   , required_argument,    0 , 'M'},
  {"remove-file"   , required_argument,    0 , 'X'},
  {"remove-all"   , no_argument,          0 , 'b'},
  {"remove-default", no_argument,          0 , 'k'},
  { "no-mask",       no_argument,          0,  'n'},
  {"default", no_argument,          0 , 'd'},
  {"recursive", no_argument,          0 , 'R'},
  {"logical", no_argument,          0 , 'L'},
  {"physical", no_argument,          0 , 'P'},
  {"version", no_argument,          0 , 'v'},
  {"help", no_argument,          0 , 'h'},
  {0,             0,                0 ,  0 }
};



int
main(int argc, char *argv[]){

  SFA_OPTIONS opt_mask;
  char * acl_in = NULL;
  int opt = 0;
  int long_index = 0;
  while((opt = getopt_long(argc,argv,"s:m:x:S:M:X:bkndRLPvh",long_options,&long_index))!=-1){
    switch (opt) {
      case 's':
        acl_in = optarg;
        opt_mask.bits.s = 1;
        break;
      case 'm':
        acl_in = optarg;
        opt_mask.bits.m = 1;
        break;
      case 'x':
        acl_in = optarg;
        opt_mask.bits.x = 1;
        break;
      case 'b':
        opt_mask.bits.b = 1;
        break;
      case 'k':
        opt_mask.bits.k = 1;
        break;
      case 'n':
        opt_mask.bits.n = 1;
        break;
      case 'd':
        opt_mask.bits.d = 1;
        break;
      case 'R':
        opt_mask.bits.R = 1;
        break;
      case 'L':
        opt_mask.bits.L = 1;
        break;
      case 'P':
        opt_mask.bits.P = 1;
        break;
      case 'v':
        opt_mask.bits.v = 1;
        break;
      case 'h':
        opt_mask.bits.h = 1;
        break;
      case '?':
        fflush(stdout);
        exit(EXIT_FAILURE);
        break;
      default:
        errExit("%s\n","error occured while parsing options");
        break;
    }
  }

  char* file_name = argv[(optind>0)? optind : optind+1];
  if(file_name==NULL)
    usageExit("%s [OPTIONS] filename\n",argv[0]);

  ACLENTRY* entries = (ACLENTRY*)malloc(sizeof(ACLENTRY*)*10);
  int num_entries
  if(opt_mask.word&SET||opt_mask.word&MODIFY||opt_mask.word&REMOVE){

    if(!short_parse_acl(acl_in,strlen(acl_in),entries,10,&num_entries)){
      free(entries);

      errnoExit("short_parse_acl()");
    }
  }

  acl_t acl;

  if((acl=acl_get_file(file,ACL_TYPE_ACCESS))==(acl_t)NULL)
    errnoExit("acl_get_file()");

  //overwrite current ACL with new ACL
  if(opt_mask.word&SET){
    acl_t new_acl; //create new acl
    if((new_acl=acl_init(num_entries))== (acl_t)NULL)
      errnoExit("acl_init()");

    int cur_index = 0; //for all ACLENTRY's
    for(;cur_index<num_entries;cur_index){
      ACLENTRY *acl_entry = (entries+cur_index);

      acl_entry_t entry; //create the entry
      if((acl_create_entry(&new_acl,&entry))!=ACL_OK)
        errnoExit("acl_create_entry()");
      //set the tag
      if((acl_set_tag_type(entry,acl_entry->tag))!=ACL_OK)
        errnoExit("acl_set_tag_type()");
      //set the qualifier if necessary
      if(acl_entry->tag == ACL_USER)
        if(acl_set_qualifier(entry,acl_entry->ids.u_qual)!=ACL_OK)
          errnoExit("acl_set_qualifier()");
      else if(acl_entry->tag == ACL_GROUP)
        if(acl_set_qualifier(entry,acl_entry->ids.g_qual)!=ACL__OK)
          errnoExit("acl_set_qualifier()");
      
      acl_permset_t permset;
      //set the permissions
      if(acl_entry->perm.nibble&READ)
        if(acl_add_perm(permset,ACL_READ)!=ACL_OK)
          errnoExit("acl_add_perm()");
      if(acl_entry->perm.nibble&WRITE)
        if(acl_add_perm(permset,ACL_WRITE)!=ACL_OK)
          errnoExit("acl_add_perm()");
      if(acl_entry->perm.nibble&EXEC)
        if(acl_add_perm(permset,ACL_EXECUTE)!=ACL_OK)
          errnoExit("acl_add_perm()");

      if(acl_set_permset(entry,permset)!=ACL_OK)
        errnoExit("acl_set_permset()");
    }
    //write to file from memory
    if(acl_set_file(file,ACL_TYPE_ACCESS,new_acl)!=ACL_OK)
      errnoExit("acl_set_file()");
  }

  free(entries);
  acl_free(acl);


  exit(EXIT_SUCCESS);


}
