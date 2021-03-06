#ifndef ACL_ENTRY_H
#define ACL_ENTRY_H

#include <sys/types.h>
#include "../../bool.h"
#include <pwd.h>
#include <grp.h>
#include <sys/acl.h>


#define MAX_ACL_ENTRIES 10


#define USER_OBJ_QUAL  (long)-1000
#define GROUP_OBJ_QUAL (long)-2000
#define OTHER_QUAL     (long)-3000
#define MASK_QUAL      (long)-4000
#define USED           (long)-5000
#define ISVALIDQUAL(x) (x!=USER_OBJ_QUAL&&x!=GROUP_OBJ_QUAL&&x!=OTHER_QUAL&&x!=MASK_QUAL)? FALSE: TRUE
#define READ  1
#define WRITE 2
#define EXEC  4

#define ACL_OK 0
#define NO_PERM -6666
#define VERIFIED_BITS 3




//holds information about entries in short form input
typedef struct _acl_entry_in{
  acl_tag_t tag; //holds the entry's tag

  //contain id_t qualifier
  union{
    uid_t u_qual; //user(used to determine if in use)
    gid_t g_qual; //group(used to determine if in use)
    long  u_obj_qual;
    long  g_obj_qual;
    long  o_qual; //other (used to determine if in use)
    long  m_qual; //mask  (used to detemine if in use)
    long  zero;
  }qualifier;

  //permission set
  union {
    struct{
      char r:1;
      char w:1;
      char x:1;
      char none:1;
    } bits;
    unsigned char nibble:4;
  } permset;

  _BOOL no_perm;

} acl_entry_in;

//partition entry by acl_tag_t(ACL_*) types
typedef struct _acl_entry_partition{
  acl_entry_in user_obj; //hold type ACL_USER_OBJ
  acl_entry_in group_obj; //holds type ACL_GROUP_OBJ
  acl_entry_in other; //holds type ACL_OTHER
  acl_entry_in mask; //holds type ACL_MASK

  acl_entry_in user[MAX_ACL_ENTRIES]; //holds type ACL_USER
  acl_entry_in group[MAX_ACL_ENTRIES];//hols type ACL_GROUP
  int num_users; //num of entries used in `user`
  int num_groups; //num of entries

}acl_entry_part;


void acl_part_init(acl_entry_part *);
_BOOL acl_short_parse(const char *,size_t, acl_entry_part *);
_BOOL acl_find_entry_with_qual(uid_t *,gid_t *,acl_entry_part *,acl_entry_in **);
_BOOL acl_create(acl_t *,acl_entry_in *);
_BOOL acl_modify(acl_entry_t *,acl_entry_in *, _BOOL, acl_t *);
_BOOL acl_find_entry_with_qual(uid_t *,gid_t *,acl_entry_part *,acl_entry_in **);
_BOOL acl_remove(acl_t *, acl_entry_t *, acl_entry_in *);
#endif
