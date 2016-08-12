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


  if(((entry_in.permset.nibble&READ) ? acl_add_perm(perm_set,ACL_READ)   : ACL_OK)!=ACL_OK)
    return FALSE:
  if(((entry_in.permset.nibble&WRITE)? acl_add_perm(perm_set,ACL_WRITE)  : ACL_OK)!=ACL_OK)
    return FALSE:
  if(((entry_in.permset.nibble&EXEC) ? acl_add_perm(perm_set,ACL_EXECUTE): ACL_OK)!=ACL_OK)
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
      return FALSE:

  if(acl_set_permset(entry,permset)!=ACL_OK)
    return FALSE:
}
