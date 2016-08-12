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
#include "acl_entry.h"
#include "setfacl.h"


/**
* update a perm_set_t in entry to the one in aclentry
* entry: acl entry to change from in-memory
* acl_entry: where to retrieve new perms from
* exits on error
**/
/*static void acl_update_perm(acl_entry_t entry,ACLENTRY *acl_entry){

  if(acl_entry == NULL) return;

  acl_permset_t perm_set;
  if(acl_get_permset(entry,&perm_set)!=ACL_OK)
    errnoExit("acl_get_perm()");

  PERM perm = acl_entry->perm;
  if(((perm.nibble&READ) ? acl_add_perm(perm_set,ACL_READ)   : ACL_OK)!=ACL_OK)
    errnoExit("acl_add_perm()");
  if(((perm.nibble&WRITE)? acl_add_perm(perm_set,ACL_WRITE)  : ACL_OK)!=ACL_OK)
    errnoExit("acl_add_perm()");
  if(((perm.nibble&EXEC) ? acl_add_perm(perm_set,ACL_EXECUTE): ACL_OK)!=ACL_OK)
    errnoExit("acl_add_perm()");

  if(acl_set_permset(entry,perm_set)!=ACL_OK)
    errnoExit("acl_set_perm()");
}*/

/**
* creates a new acl_entry_t given acl and acl_entry
* acl: ptr to acl to add to
* acl_entry: where to get tag,qualifier, and perms
* exits on error
**/
/*static void acl_create(acl_t *acl,ACLENTRY *acl_entry){
  acl_entry_t entry; //create the entry
  if((acl_create_entry(acl,&entry))!=ACL_OK)
    errnoExit("acl_create_entry()");
  //set the tag
  if((acl_set_tag_type(entry,acl_entry->tag))!=ACL_OK)
    errnoExit("acl_set_tag_type()");
  //set the qualifier if necessary
  if(acl_entry->tag == ACL_USER)
    if(acl_set_qualifier(entry,&acl_entry->ids.u_qual)!=ACL_OK)
      errnoExit("acl_set_qualifier()");
  else if(acl_entry->tag == ACL_GROUP)
    if(acl_set_qualifier(entry,&acl_entry->ids.g_qual)!=ACL_OK)
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
}*/





/**
*sets an a new acl
*file: file whose acl to change
*list: list of entries to make new acl
*num_entries: number of entries
* exits on error
**/
/*void acl_set(const char *file,ACLENTRY *list, int num_entries){
  acl_t acl;

  if((acl=acl_get_file(file,ACL_TYPE_ACCESS))==(acl_t)NULL)
    errnoExit("acl_get_file()");

  acl_t new_acl; //create new acl
  if((new_acl=acl_init(num_entries))== (acl_t)NULL)
    errnoExit("acl_init()");

  int cur_index = 0; //for all ACLENTRY's
  for(;cur_index<num_entries;cur_index){
    ACLENTRY *acl_entry = (list+cur_index);
    acl_create(&new_acl,acl_entry);
  }
  //write to file from memory
  if(acl_set_file(file,ACL_TYPE_ACCESS,new_acl)!=ACL_OK)
    errnoExit("acl_set_file()");
  acl_free(acl);
  acl_free(new_acl);
}*/


/**
* modifies an acl to contain entries in 'list'
* file: file whose acl to modify
* list: list of entries to add
* num_entries: number of entries in `list`
* exits on error
**/
/*void acl_mod(const char *file,ACLENTRY *list, int num_entries){

  acl_t acl;

  if((acl=acl_get_file(file,ACL_TYPE_ACCESS))==(acl_t)NULL)
    errnoExit("acl_get_file()");



  ACLENTRY *user_obj  = NULL; //new user_obj
  ACLENTRY *group_obj = NULL; //new group_obj
  ACLENTRY *other     = NULL; //new other
  ACLENTRY *mask      = NULL; //new mask
  ACLENTRY *user[MAX_ACL_ENTRIES]; //new users
  ACLENTRY *group[MAX_ACL_ENTRIES]; //new groups
  int user_ind  = 0;
  int group_ind = 0;

  if(!acl_partition(list,num_entries,&user_obj,&group_obj,&other,&mask,user,group,MAX_ACL_ENTRIES,MAX_ACL_ENTRIES,&user_ind,&group_ind))
      errnoExit("acl_partition()");

  int cur_index = 0;
  int entry_id = ACL_FIRST_ENTRY;
  acl_entry_t entry; //go through all current acl entries
  uid_t *uid;
  gid_t *gid;
  for(;acl_get_entry(acl,entry_id,&entry)!=NO_MORE_ENTRIES;entry_id=ACL_NEXT_ENTRY){
      acl_tag_t tag;
      if(acl_get_tag_type(entry,&tag)!=ACL_OK)
        errnoExit("acl_get_tag_type()");

      switch (tag) {
        case ACL_USER_OBJ: //update the user obj
          acl_update_perm(entry,user_obj);
          break;
        case ACL_GROUP_OBJ://update the group obj
          acl_update_perm(entry,group_obj);
          break;
        case ACL_MASK: //update the mask
          acl_update_perm(entry,mask);
          break;
        case ACL_OTHER: //update the other
          acl_update_perm(entry,other);
          break;
        case ACL_USER: //update all users

          if((uid=acl_get_qualifier(entry))==(void *)NULL)
            errnoExit("acl_get_qualifier()");
          cur_index=0;
          for(;cur_index<user_ind;cur_index++){
            if(user[cur_index]->ids.u_qual==*uid){ //modify user entry if told to
              acl_update_perm(entry,user[cur_index]);
              user[cur_index]->ids.marked = -1;
              break;
            }
          }
          break;
        case ACL_GROUP: //update all groups
          if((gid=acl_get_qualifier(entry))==(void *)NULL)
            errnoExit("acl_get_qualifier()");
          cur_index=0;
          for(;cur_index<group_ind;cur_index++){
            if(group[cur_index]->ids.g_qual==*gid){ //modify group entry if told to
              acl_update_perm(entry,group[cur_index]);
              group[cur_index]->ids.marked = -1;
              break;
            }
          }
          break;
        default:
          errExit("%s %ld\n","Received unexpected tag:",(long)tag);
          break;
      }
    }

  cur_index =0; //for entries not used (i.e. not currently in the acl) all them
  for(;cur_index<user_ind;cur_index++)
    if(user[cur_index]->ids.marked!=-1)
        acl_create(&acl,user[cur_index]);
  cur_index =0; //same as above but for groups
  for(;cur_index<group_ind;cur_index++)
    if(group[cur_index]->ids.marked!=-1)
        acl_create(&acl,group[cur_index]);

  //write to file from memory
  if(acl_set_file(file,ACL_TYPE_ACCESS,acl)!=ACL_OK)
    errnoExit("acl_set_file()");
  acl_free(acl);


}


void rem_acl(const char *file,ACLENTRY *list, int num_entries){
  acl_t acl;

  if((acl=acl_get_file(file,ACL_TYPE_ACCESS))==(acl_t)NULL)
    errnoExit("acl_get_file()");


    ACLENTRY *user_obj  = NULL; //new user_obj
    ACLENTRY *group_obj = NULL; //new group_obj
    ACLENTRY *other     = NULL; //new other
    ACLENTRY *mask      = NULL; //new mask
    ACLENTRY *user[MAX_ACL_ENTRIES]; //new users
    ACLENTRY *group[MAX_ACL_ENTRIES]; //new groups
    int user_ind  = 0;
    int group_ind = 0;

    if(!acl_partition(list,num_entries,&user_obj,&group_obj,&other,&mask,user,group,MAX_ACL_ENTRIES,MAX_ACL_ENTRIES,&user_ind,&group_ind))
        errnoExit("acl_partition()");

    int entry_id = ACL_FIRST_ENTRY;
    acl_entry_t entry; //go through all current acl entries

    for(;acl_get_entry(acl,entry_id,&entry)!=NO_MORE_ENTRIES;entry_id=ACL_NEXT_ENTRY){




    }
}*/



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

  acl_entry_part part;
  acl_part_init(&part);

  if(!acl_short_parse(acl_in,strlen(acl_in),&part))
    errnoExit("short_parse_acl()");

  /*
  ACLENTRY* entries = (ACLENTRY*)malloc(sizeof(ACLENTRY*)*10);
  int num_entries
  if(opt_mask.word&SET||opt_mask.word&MODIFY||opt_mask.word&REMOVE){

    if(!short_parse_acl(acl_in,strlen(acl_in),entries,10,&num_entries)){
      free(entries);

      errnoExit("short_parse_acl()");
    }
  }*/



  //overwrite current ACL with new ACL
/*  if(opt_mask.word&SET){

  }

  else if(opt_mask.word&MODIFY){

  }*/

  //free(entries);



  exit(EXIT_SUCCESS);


}
