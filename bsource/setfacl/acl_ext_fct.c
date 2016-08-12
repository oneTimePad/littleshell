#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/acl.h>
#include "acl_entry.h"
#include "acl_ext_fct.h"




/**
*sets a new acl
*file: file whose acl to change
*acl_part: ptr to acl_entry_part
*returns: status
**/
_BOOL acl_set(const char *file,acl_entry_part *acl_part){
  acl_t acl;

  if((acl=acl_get_file(file,ACL_TYPE_ACCESS))==(acl_t)NULL)
    return FALSE;

  acl_t new_acl; //create new acl
  if((new_acl=acl_init(INIT_ENTRIES))== (acl_t)NULL)
    return FALSE

  if(acl_part->user_obj.qualifier.u_qual == USER_OBJ_QUAL) //check if there is user_obj entry to set
    if(!acl_create(&new_acl,&acl_part->user_obj))
      return FALSE;
  if(acl_part->group_obj.qualifier.g_qual == GROUP_OBJ_QUAL) //check if there is a group_obj entry to set
    if(!acl_create(&new_acl,&acl_part->group_obj))
      return FALSE;
  if(acl_part->other.qualifier.o_qual == OTHER_QUAL) //check if there is an other entry to set
    if(!acl_create(&new_acl,&acl_part->other))
      return FALSE;
  if(acl_part->mask.qualifier.m_qual == MASK_QUAL) //check if there is a mask entry to set
    if(!acl_create(&new_acl,&acl_part->mask))
      return FALSE;

  int entry_ind =0;
  int num_users = acl_part->num_users;
  for(;entry_ind<num_users;entry_ind++) //set user entries if any
    if(!acl_create(&new_acl,&acl_part->user[entry_ind]))
      return FALSE;
  entry_ind = 0;
  int num_groups = acl_partition->num_groups;
  for(;entry_id<num_groups;entry_id++) //set group entries if any
    if(!acl_create(&new_acl,&acl_part->group[entry_ind]))
      return FALSE;

  //write to file from memory
  if(acl_set_file(file,ACL_TYPE_ACCESS,new_acl)!=ACL_OK)
    return FALSE;

  if(acl_free(&acl)!=ACL_OK)
    return FALSE;
  if(acl_free(&new_acl)!=ACL_OK)
    return FALSE;

  return TRUE;
}

/**
*modifies `file`'s acl with entries in `acl_part`
* file: file whose acl to modify
* acl_part: ptr to acl partition
* returns: status
**/
_BOOL acl_mod(const char *file, acl_entry_part *acl_part){
  acl_t acl;

  if((acl=acl_get_file(file,ACL_TYPE_ACCESS))==(acl_t)NULL)
    errnoExit("acl_get_file()");

  int entry_ind = ACL_FIRST_ENTRY;
  acl_entry_t entry;
  //loop through current entries
  for(;acl_get_entry(acl,entry_ind,&entry)!=NO_MORE_ENTRIES;entry_ind = ACL_NEXT_ENTRY){
    acl_tag_t tag;
    if(acl_get_tag_type(entry&tag)!=ACL_OK)
      return FALSE;
    uid_t *uid;
    gid_t *gid;
    switch (tag) {
      case ACL_USER_OBJ: //modifies the user_obj if told to
        if(acl_part->user_obj.qualifier.u_qual != USER_OBJ_QUAL)
          break;
        if(!acl_mod(&entry,&acl_part->user_obj,FALSE,NULL))
          return FALSE;
        break;
      case ACL_GROUP_OBJ: //modifiers the group_obj if told to
        if(acl_part->group_obj.qualifier.g_qual != GROUP_OBJ_QUAL)
          break;
        if(!acl_mod(&entry,&acl_part->group_obj,FALSE,NULL))
          return FALSE;
        break;
      case ACL_OTHER: //modifies other if told to
        if(acl_part->other.qualifier.o_qual != OTHER_QUAL)
          break;
        if(!acl_mod(&entry,&acl_part->other))
          return FALSE;
        break;
      case ACL_MASK: //modifiers mask if told to
        if(acl_part->mask.qualifier.m_qual != MASK_QUAL)
          break;
        if(!acl_mod(&entry,&acl_part->mask))
          return FALSE;
          break;
      case ACL_USER: //modifies user if told to
        if((uid = acl_get_qualifier(entry))==NULL)
          return FALSE;
        acl_entry_in *entry_in = NULL;
        if(!acl_part_entry_with_qual(uid,NULL,acl_part,&entry_in))
          return FALSE;
        entry->qualifier.zero = USED;
        if(!acl_modify(&entry,entry_in,FALSE,NULL))
          return FALSE;
        break;
      case ACL_GROUP: //modifiers group if told to
        if((gid = acl_get_qualifier(entry))==NULL)
          return FALSE;
        acl_entry_in *entry_in = NULL;
        if(!acl_part_entry_with_qual(NULL,gid,acl_part,&entry_in))
          return FALSE;
        entry->qualifier.zero = USED;
        if(!acl_modify(&entry,entry_in,FALSE,NULL))
          return FALSE;
        break
      default:
        errno = EINVAL;
        return FALSE;
    }
  }
  //add ones that are not modifying existing entries
  entry_ind = 0;
  int num_entries = acl_part->num_users;
  for(;entry_id<num_entries;entry_id++){
    if(acl_part->user[entry_ind].qualidier.zero!=USED){
      if(!acl_modify(NULL,entry,TRUE,&acl))
        return FALSE;
    }
  }
  //same as above but for groups
  entry_ind = 0;
  num_entries = acl_part->num_groups;
  for(;entry_id<num_entries;entry_id++){
    if(acl_part->group[entry_ind].qualidier.zero!=USED){
      if(!acl_modify(NULL,entry,TRUE,&acl))
        return FALSE;
    }
  }

  if(acl_free(&acl)!=ACL_OK)
    return FALSE;

}
