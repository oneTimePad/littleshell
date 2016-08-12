#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include "acl_entry.h"


/**
* converts name into uid equiv
* pw_name : username
* returns: uid or -1 on error
**/
static inline uid_t getuidfromname(const char *pw_name){
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
static inline gid_t getgidfromname(const char *gr_name){
  struct group* gr;
  if((gr=getgrnam(gr_name))==NULL)
    return (gid_t)-1;
  return gr->gr_gid;
}


/**
* set an acl_entry_in's permset pased on the 'rwx' in perm_string
*perm_string: string of the form r{-}w{-}x{-}
* entry: ptr to acl_entry_in whose permset to fill
* sets errno to EINVAL on error
* returns: status
**/
static _BOOL set_permset(char **perm_string,acl_entry_in *entry){
  if(perm_string==NULL || *perm_string == NULL || **perm_string=='\0'){
    errno = EINVAL;
    return FALSE;
  }


  entry->permset.bits.r = (*(*perm_string)++=='r') ? 1 : 0;
  if(*perm_string=='\0'){ //make sure the string didn't end for some reason
    errno = EINVAL;
    return FALSE;
  }
  entry->permset.bits.w = (*(*perm_string)++=='w')? 1 : 0;
  if(*perm_string=='\0'){
    errno = EINVAL;
    return FALSE;
  }
  entry->permset.bits.x = (*(*perm_string)=='x') ? 1 : 0;

  return TRUE;

}

static inline void acl_entry_init(acl_entry_in *entry){
    entry->tag = (long)-1;
    entry->qualifier.zero = (long)-1;
    entry->permset.nibble = 0;
}

void acl_part_init(acl_entry_part *acl_part){
    acl_entry_init(&acl_part->user_obj);
    acl_entry_init(&acl_part->group_obj);
    acl_entry_init(&acl_part->other);
    acl_entry_init(&acl_part->mask);
    acl_part->num_users = 0;
    acl_part->num_groups = 0;

    int entry_index=0;
    for(;entry_index<MAX_ACL_ENTRIES;entry_index++){
      acl_entry_init(&acl_part->user[entry_index]);
      acl_entry_init(&acl_part->group[entry_index]);
    }
}

/**
* parsing short-form acl string
* acl_string: acl entries string in short form
* string_size: len of acl_string
* acl_part: ptr to acl_entry_part to fill
* errno: sets errno to EINVAL if string is malformed, or ENOMEM if no more entries available
*   errno is also set by getuidfromname and getgidfromname
* returns: status and sets errno
**/
_BOOL acl_short_parse(const char* acl_string,size_t string_size, acl_entry_part *acl_part){
  if( string_size<=0 ||acl_string==NULL || *acl_string == '\0' ) return FALSE;
  if(strlen(acl_string)!=string_size) return FALSE;
  //copy over the string
  char acl_string_copy[string_size+1];
  strncpy(acl_string_copy,acl_string,string_size);
  acl_string_copy[string_size]= '\0';
  char * acl_string_cpy = acl_string_copy; //need ptr arithmetic

  volatile acl_entry_in *current_entry = NULL;
  char *qual_str = NULL;
  //loop through short form acl string
  for(;*acl_string_cpy!='\0';acl_string_cpy++){
      if(*acl_string_cpy==',')
        continue;
      //set the tag
      switch (*acl_string_cpy++) {
        case 'u': //since we don't know if a qual is specified, we just set to OBJ for now
          if(*acl_string_cpy++!=':') {errno = EINVAL; return FALSE;}
          acl_entry_in *user_entry = NULL;
          qual_str = acl_string_cpy;
          if(*qual_str!=':'){
            while(*qual_str!=':'){
                // keep moving till we find a ':' or '\0'
              if(*qual_str++ == '\0'){errno = EINVAL; return FALSE;}
            }
            *qual_str='\0';
            user_entry =&acl_part->user[acl_part->num_users++];
            //convert the qualifier name to uid (reason we added '\0')
            if((user_entry->qualifier.u_qual=getuidfromname(acl_string_cpy))==-1){
            if(errno ==0)
              errno = ENOENT;
              return FALSE;
            }
            *qual_str=':';
            acl_string_cpy = qual_str+1;
            user_entry->tag = ACL_USER;
          }
          else{
            acl_string_cpy++;
            user_entry = &acl_part->user_obj;
            user_entry->qualifier.u_qual = USER_OBJ_QUAL;
            user_entry->tag = ACL_USER_OBJ;

          }
          if(!set_permset(&acl_string_cpy,user_entry)) return FALSE;
          break;
        case 'g': //same reason for 'u'
          if(*acl_string_cpy++!=':') {errno = EINVAL; return FALSE;}
          acl_entry_in *group_entry = NULL;
          qual_str = acl_string_cpy;
          if(*qual_str!=':'){
            while(*qual_str!=':'){
            // keep moving till we find a ':' or '\0'
            if(*qual_str++ == '\0'){errno = EINVAL; return FALSE;}
            }
            *qual_str='\0';
            group_entry =&acl_part->group[acl_part->num_groups++];
            //convert the qualifier name to uid (reason we added '\0')
            if((group_entry->qualifier.g_qual=getgidfromname(acl_string_cpy))==-1){
            if(errno ==0)
            errno = ENOENT;
            return FALSE;
            }
            *qual_str=':';
            acl_string_cpy = qual_str+1;
            group_entry->tag = ACL_GROUP;
          }
          else{
            acl_string_cpy++;
            group_entry = &acl_part->group_obj;
            group_entry->qualifier.g_qual = GROUP_OBJ_QUAL;
            group_entry->tag = ACL_GROUP_OBJ;
          }
          if(!set_permset(&acl_string_cpy,group_entry)) return FALSE;
            break;
        case 'o': // other
          if(*acl_string_cpy++!=':'||*acl_string_cpy++!=':') {errno = EINVAL; return FALSE;}
          acl_part->other.qualifier.o_qual = OTHER_QUAL;
          acl_part->other.tag = ACL_OTHER;
          if(!set_permset(&acl_string_cpy,&acl_part->other)) return FALSE;
          break;
        case 'm': //mask
          if(*acl_string_cpy++!=':'||*acl_string_cpy++!=':') {errno = EINVAL; return FALSE;}
          acl_part->mask.qualifier.m_qual = MASK_QUAL;
          acl_part->mask.tag = ACL_MASK;
          if(!set_permset(&acl_string_cpy,&acl_part->mask)) return FALSE;
          break;
        default: // it is something unknown
          errno = EINVAL;
          return FALSE;
          break;
      }
    }
    return TRUE;
}




/**
* update a perm_set_t in entry to the one in aclentry
* entry: acl entry to change from in-memory
* acl_entry: where to retrieve new perms from
* returns status
**/
static _BOOL acl_update_perm(acl_entry_t entry,acl_entry_in *entry_in){

  if(entry_in == NULL) return;

  acl_permset_t perm_set;
  if(acl_get_permset(entry,&perm_set)!=ACL_OK)
    return FALSE;


  if(((entry_in->permset.nibble&READ) ? acl_add_perm(perm_set,ACL_READ)   : ACL_OK)!=ACL_OK)
    return FALSE;
  if(((entry_in->permset.nibble&WRITE)? acl_add_perm(perm_set,ACL_WRITE)  : ACL_OK)!=ACL_OK)
    return FALSE;
  if(((entry_in->permset.nibble&EXEC) ? acl_add_perm(perm_set,ACL_EXECUTE): ACL_OK)!=ACL_OK)
    return FALSE;

  if(acl_set_permset(entry,perm_set)!=ACL_OK)
    return FALSE;
}

/**
* creates a new acl_entry_t given acl and acl_entry
* acl: ptr to acl to add to
* acl_entry: where to get tag,qualifier, and perms
* returns status
**/
_BOOL acl_create(acl_t *acl,acl_entry_in *entry_in){
  acl_entry_t entry; //create the entry
  if((acl_create_entry(acl,&entry))!=ACL_OK)
    return FALSE;
  //set the tag
  if((acl_set_tag_type(entry,entry_in->tag))!=ACL_OK)
    return FALSE;
  //set the qualifier if necessary
  if(entry_in->tag == ACL_USER)
    if(acl_set_qualifier(entry,&entry_in->qualifier.u_qual)!=ACL_OK)
      return FALSE;
  else if(entry_in->tag == ACL_GROUP)
    if(acl_set_qualifier(entry,&entry_in->qualifier.g_qual)!=ACL_OK)
      return FALSE;

  acl_permset_t permset;
  //set the permissions
  if(entry_in->permset.nibble&READ)
    if(acl_add_perm(permset,ACL_READ)!=ACL_OK)
      return FALSE;
  if(entry_in->permset.nibble&WRITE)
    if(acl_add_perm(permset,ACL_WRITE)!=ACL_OK)
      return FALSE;
  if(entry_in->permset.nibble&EXEC)
    if(acl_add_perm(permset,ACL_EXECUTE)!=ACL_OK)
      return FALSE;

  if(acl_set_permset(entry,permset)!=ACL_OK)
    return FALSE;
}
