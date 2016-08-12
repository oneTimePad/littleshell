#ifndef ACL_EXT_FCT_H
#define ACL_EXT_FCT_H
#include "acl_entry.h"

#define INIT_ENTRIES 10
#define NO_MORE_ENTRIES 1

_BOOL acl_set(const char *,acl_entry_part *); //used by -s/S flag
_BOOL acl_mod(const char *,acl_entry_part *); //used by -m/M flag
_BOOL acl_rem(const char *,acl_entry_part *); //used by -x/X flag


#endif
