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
#include "acl_ext_fct.h"
#include "setfacl.h"





static struct option long_options[] = {
  {"set", required_argument,    0 , 's'},
  {"modify"  , required_argument,    0 , 'm'},
  {"remove" , required_argument,    0 , 'x'},
  {"set-file"   , required_argument,    0 , 'S'},
  {"Lodify-file"   , required_argument,    0 , 'M'},
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
  opt_mask.word = 0;
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

  acl_entry_part acl_part;
  acl_part_init(&acl_part);

  if(!acl_short_parse(acl_in,strlen(acl_in),&acl_part))
    errnoExit("short_parse_acl()");



  //overwrite current ACL with new ACL
  if(opt_mask.word&SET){

    if(!acl_set(file_name,&acl_part)){
      if(errno == -1){
        errno =0;
        errExit("%s\n","the ACL that is replacing the current ACL is invalid!");
      }
      errnoExit("acl_set()");
    }
  }
  //modify current ACL with new ACL
  else if(opt_mask.word&MODIFY){

    if(!acl_mod(file_name,&acl_part)){
      if(errno == -1){
        errno =0;
        errExit("%s\n","the ACL that is replacing the current ACL is invalid!");
      }
      errnoExit("acl_mod()");
    }

  }
  //remove entries from curent with that are in input ACL
  else if(opt_mask.word&REMOVE){

    if(!acl_rem(file_name,&acl_part)){
      if(errno == -1){
        errno =0;
        errExit("%s\n","the ACL that is replacing the current ACL is invalid!");
      }
      errnoExit("acl_rem()");
    }
  }
  else{
    errExit("%s\n","unimplemented option");
  }




  exit(EXIT_SUCCESS);


}
