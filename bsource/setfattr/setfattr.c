#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/xattr.h>
#include "setfattr.h"


/**
* validate options MASK
* options: ptr to options bit MASK
* returns: status
**/
static _BOOL validate_mask(SXA_OPTIONS *options){

  _BOOL validate = FALSE;
  validate =  (options->byte&NAME && !options->byte&VALUE || !options->byte&NAME && !options->byte&VALUE||
    ((options->byte&NAME ||options->byte&VALUE)&&(options->byte&REMOVE)) ) ? FALSE : TRUE;
  if(!validate) // can't have -n without -v and viseversa
    return validate;
  validate =  (options->byte&REMOVE &&(options->byte&NAME || options->byte&VALUE)) ? FALSE : TRUE;
  return validate;


}




static struct option long_options[] = {
  {"name", required_argument,    0 , 'n'},
  {"value"  , required_argument,    0 , 'v'},
  {"remove" , required_argument,    0 , 'x'},
  {"no-dereference"   , no_argument,    0 , 'd'},
  {"restore"   , required_argument,    0 , 'r'},
  {"help", no_argument,          0 , 'h'},
  {"replace",no_argument,       0,    'R'},
  {0,             0,                0 ,  0 }
};


int
main(int argc, char *argv[]){


  SXA_OPTIONS opt_mask;
  opt_mask.byte = 0;
  int opt = 0;
  int long_index = 0;
  char *name = NULL;
  char *value = NULL;
  while((opt = getopt_long(argc,argv,"n:v:x:drhR",long_options,&long_index))!=-1){
    switch (opt) {
      case 'n':
        opt_mask.bits.n = 1;
        name = optarg;
        break;
      case 'v':
        opt_mask.bits.v = 1;
        value = optarg;
        break;
      case 'x':
        opt_mask.bits.x = 1;
        name = optarg;
        break;
      case 'd':
        opt_mask.bits.d = 1;
        break;
      case 'r':
        opt_mask.bits.r = 1;
        break;
      case 'h':
        opt_mask.bits.h = 1;
        break;
      case 'R':
        opt_mask.bits.R = 1;
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

  if(!validate_mask(&opt_mask))
    errExit("%s\n","invalid options combination");
    //add an attribute
  if(opt_mask.byte&NAME){

    char value_buf[strlen(value)+1];
    strcpy(value_buf,value);
    int status = (opt_mask.byte&NO_DEREF) ? lsetxattr(file_name,name,value_buf,strlen(value)+1,(opt_mask.byte&REPLACE) ? XATTR_REPLACE : XATTR_CREATE) :
                                            setxattr(file_name,name,value_buf,strlen(value)+1,(opt_mask.byte&REPLACE) ? XATTR_REPLACE : XATTR_CREATE) ;
    if(status!=XATTR_OK){
      errnoExit((opt_mask.byte&NO_DEREF)? "lsetxattr()" : "setxattr()");
    }

  }
  //remove an attribute
  else if(opt_mask.byte&REMOVE){
    int status = (opt_mask.byte&NO_DEREF) ? lremovexattr(file_name,name) : removexattr(file_name,name);

    if(status!=XATTR_OK){
      errnoExit((opt_mask.byte&NO_DEREF)? "lremovexattr()" : "removexattr()");
    }
  }
  else{
    errExit("%s\n","unimplemented options");
  }

  exit(EXIT_SUCCESS);
}
