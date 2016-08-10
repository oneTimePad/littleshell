


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "setfacl.h"


static inline uid_t getuidfromname(const char *pw_name){
  struct passwd* pw;
  if((pw=getpwnam(pw_name))==NULL)
    return (uid_t)-1;
  return pw->pw_uid;
}

static inline gid_t getgidfromname(const char *gr_name){
  struct group* gr;
  if((grp=getgrnam(gr_name))==NULL)
    return (gid_t)-1;
  return gr->gr_gid;
}

/**
* parsing short-form acl string
* acl_string: acl entries string in short form
* list: list containing acl structs
* num_entries: size of list in terms of number of acl entries
* sets errno to EINVAL if string is malformed, or ENOMEM if no more entries available
* errno is also set by getuidfromname and getgidfromname
* returns: status and sets errno
**/
static _BOOL short_parse_acl(const char* acl_string,size_t string_size, ACLENTRY* list,int num_entries){
  if(list == NULL || string_size==0 ||acl_string==NULL || *acl_list == '\0' || size ==0) return FALSE;

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
  for(;*acl_string_cpy!='\0',acl_string_cpy++){
    //if a comma is seen move to new entry
    if(*acl_string_cpy==','){
      cur_entry++;
      if(cur_entry>=num_entries){
        errno = ENOMEM;
        return FALSE;
      }
    }
    //if the next char is a ':' and the tag isn't set yet
    else if(*(acl_string_cpy+1) ==':'&&!set_tag){
      //set the tag
      switch (*acl_string_cpy) {
        case 'u': //since we don't know if a qual is specified, we just set to OBJ for now
          (list+cur_entry)->tag = ACL_USER_OBJ;
          break;
        case 'g': //same reason for 'u'
          (list+cur_entry)->tag = ACL_GROUP_OBJ;
          break;
        case 'o': // other
          (list+cur_entry)->tag = ACL_OTHER;
          break;
        case 'm': //mask
          (list+cur_entry)->tag = ACL_MASK;
          break;
        default: // it is something unknown
          errno = EINVAL;
          return FALSE;
          break;
      }
      acl_string_cpy++;
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
      do {
        qual_start++; // keep moving till we find a ':' or '\0'
      } while(*qual_start!=':' && *qual_start!='\0');
      if(*qual_start=='\0'){ //the string isn't formatted corectly
        errno = EINVAL;
        return FALSE;
      }
      *qual_start = '\0'; //end the qualifier with a (change ':' to) '\0' (reason necessary is apparent below)
      if((list+cur_entry)->tag == ACL_USER_OBJ){ //if the tag was user
        uid_t qual; //convert the qualifier name to uid (reason we added '\0')
        if((qual=getuidfromname(acl_string_cpy))==-1)
          return FALSE;
        (list+cur_entry)->u_qual = qual;
        (list+cur_entry)->g_qual = -1; //unused
        (list+cur_entry)->tag = ACL_USER; //changed tag since qualifier was specified
      }
      else if((list+cur_entry)->tag == ACL_GROUP_OBJ){ //if the tag was group
        gid_it qual;//convert the qualifier name to gid (reason we added '\0')
        if((qual=getgidfromname(acl_string_cpy))==-1)
          return FALSE;
        (list+cur_entry)->u_qual = -1; //unused
        (list+cur_entry)->g_qual = qual;
        (list+cur_entry)->tag = ACL_GROUP; //changed tag since qualifier was specified
      }
      *qual_start = ':'; //remove the '\0', put it back to ':'
      acl_string_cpy = qual_start;
      set_qual = TRUE;
    }
    //if the tag and qualifier are both set
    else if(set_tag&&set_qual){
      //set the permissions mask (expects 'rwx' order else undefined result)
      (list+cur_entry)->perm.r = (*acl_string_cpy++&READ) ? 1 : 0;
      if(*acl_string_cpy=='\0'){ //make sure the string didn't end for some reason
        errno = EINVAL;
        return FALSE;
      }
      (list+cur_entry)->perm.w = (*acl_string_cpy++&WRITE)? 1 : 0;
      if(*acl_string_cpy=='\0'){
        errno = EINVAL;
        return FALSE;
      }
      (list+cur_entry)->perm.x = (*acl_string_cpy++&EXEC)   ? 1 : 0;
    }
    //else something unexpected was seen in the string
    else{
      errno = EINVAL;
      return FALSE;
    }
  }

}

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


}
