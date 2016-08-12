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
  if(perm_string==NULL || *perm_string == NULL){
    errno = EINVAL;
    return FALSE;
  }

  if(**perm_string=='\0'){
    entry->no_perm = TRUE;
    return TRUE;
  }


  entry->no_perm = FALSE;
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

/**
* sets up acl_entry_in with default values
* entry: ptr to acl_entry_in to initialize
* always succeeds
**/
static inline void acl_entry_init(acl_entry_in *entry){
    entry->tag = (long)-1;
    entry->qualifier.zero = (long)-1;
    entry->permset.nibble = 0;
}

/**
* set ups acl_part with default values
* acl_part: ptr to acl_part to initialize
* aclways succeeds
**/
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
static _BOOL acl_update_perm(acl_entry_t *entry,acl_entry_in *entry_in){

  if(entry_in == NULL) return;

  acl_permset_t perm_set;
  if(acl_get_permset(*entry,&perm_set)!=ACL_OK)
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

static _BOOL acl_update_permset(acl_entry_t *entry,acl_entry_in *entry_in){
  if(entry_in->no_perm){
    errno = EINVAL;
    return FALSE;
  }
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
  return TRUE;
}

/**
* looks for a user entry with uid if uid is not NULL or group entry is gid is not NULL
* uid: ptr to uid qualifier
* gid: ptr to gid qualidier
* acl_part: ptr to partition of acl_entry_in's
* acl_entry_in: double pointer that outputs found entry
* returns: status sets errno on error
**/
_BOOL acl_find_entry_with_qual(uid_t *uid,gid_t *gid,acl_entry_part *acl_part,acl_entry_in **entry_in){
  if((uid==NULL &&gid ==NULL)acl_part==NULL || entry_in == NULL){errno = EINVAL; return FALSE;}
  _BOOL use_uid = (uid==NULL) ? FALSE : TRUE;


  int entry_ind =0;
  entry_in *found = NULL;
  int num_entries = (use_uid) ? acl_part->num_users : acl_part->num_groups;
  for(;entry_ind<num_entries;entry_ind++){
    long qual = (use_uid) ? acl_part->user[entry_ind].qualidier.u_qual : acl_part->group[entry_ind].qualifier.g_qual;
    if(qual == (long)((use_uid) ? *uid: *gid)){
      found = (use_uid) ? &acl_part->user[entry_ind] : &acl_part->group[entry_ind];
      break;
    }
  }

  if(found == NULL){
    errno = ENOENT;
    return FALSE;
  }

  *entry_in = found;
  return TRUE;

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

  if(!acl_update_permset(&entry,entry_in))
    return FALSE;
  return TRUE;
}

/**
* modifies `entry` to contain characteristics of `entry_in`
*entry: ptr to entry to modify(only used if !add is true)
*entry_in: ptr to entry who characterisitcs to use to modify entry
*add: should this be a new entry
* acl: acl to add new entry to
*returns: status
* if add is FALSE, acl is not checked
**/
_BOOL acl_modify(acl_entry_t *entry,acl_entry_in *entry_in,_BOOL add,acl_t *acl){
  if(entry_in == NULL){errno = EINVAL; return FALSE;}
  if(!add&&entry==NULL){errno = EINVAL;return FALSE;}
  if(add&&acl==NULL){errno = EINVAL; return FALSE;}
  if(!add){
    //just do some checks to verify input


     acl_tag_t tag;
     if(acl_get_tag_type(*entry,&tag)!=ACL_OK)
      return FALSE;

     if(tag!=entry_in->tag){errno = EINVAL; return FALSE;}

     long *qual;
     if((qual=acl_get_qualifier(*entry))!=NULL){
       if(*qual!=entry_in->qualifier.zero)
        return FALSE
     }
     else if(!ISVALIDQUAL(entry_in->qualifier.zero))
      return FALSE;
    //end checks

    if(!acl_update_permset(&entry,entry_in))
      return FALSE;
  }
  else if(add){
    if(!acl_create(acl,entry_in))
      return FALSE;
  }

  return TRUE;

}

/**
* removes entry in ptr from acl
* acl: ptr to acl to modify
* entry: ptr to entry to remove
* returns: status
**/
_BOOL acl_remove(acl_t * acl, acl_entry_t *entry){
  if(acl==NULL || entry==NULL){errno = EINVAL; return FALSE;}

  if(acl_delete_entry(entry)!=ACL_OK)
    return FALSE;

  return TRUE;

}
