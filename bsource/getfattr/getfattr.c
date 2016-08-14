#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sys/xattr.h>
#include <libgen.h>
#include "getfattr.h"



/**
* validate options MASK
* options: ptr to options bit MASK
* returns: status
**/
static inline _BOOL validate_mask(GXA_OPTIONS *options){

  _BOOL validate = FALSE;
  validate = (options->nibble&NAME && options->nibble&DUMP) ? FALSE : TRUE;
  return validate;

}



static struct option long_options[] = {
  {"name", required_argument,    0 , 'n'},
  {"dump"  , no_argument,    0 , 'd'},
  {"no-dereference" , no_argument,    0 , 'h'},
  {0,             0,                0 ,  0 }
};



int
main(int argc, char *argv[]){

  GXA_OPTIONS opt_mask;
  opt_mask.nibble = 0;
  int opt = 0;
  int long_index = 0;
  char *name = NULL;
  char *value = NULL;
  while((opt = getopt_long(argc,argv,"n:dh",long_options,&long_index))!=-1){
    switch (opt) {
      case 'n':
        opt_mask.bits.n = 1;
        name = optarg;
        break;
      case 'd':
        opt_mask.bits.d = 1;
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

  if(!validate_mask(&opt_mask))
    errExit("%s\n","invalid options combination");
  printf("# file: %s\n",basename(file_name));

  //find an attribute by name
  if(opt_mask.nibble&NAME){
      ssize_t size = (opt_mask.nibble&NO_DEREF) ? lgetxattr(file_name,name,NULL,0) : getxattr(file_name,name,NULL,0);
      char value_buf[size*4];
      int status = (opt_mask.nibble&NO_DEREF) ? lgetxattr(file_name,name,value_buf,size*4) : getxattr(file_name,name,value_buf,size*4);
      if(status!=XATTR_OK){
        errnoExit((opt_mask.nibble&NO_DEREF)? "lgetxattr()" : "getxattr()");
      }
      printf("%s=%s\n",name,value_buf);
  }
  //dump all attributes
  else if(opt_mask.nibble&DUMP){
      ssize_t size = (opt_mask.nibble&NO_DEREF) ? llistxattr(file_name,NULL,0) : listxattr(file_name,NULL,0);
      char name_buf[size*4];//a possibly bad assumption, but for protection
      int status = (opt_mask.nibble&NO_DEREF) ? llistxattr(file_name,name_buf,size*4) : listxattr(file_name,name_buf,size*4);
      if(status!=XATTR_OK){
        errnoExit((opt_mask.nibble&NO_DEREF)? "llistxattr()" : "listxattr()");
      }

      int str_start = 0; //fetch all attribute names
      for(;str_start<size; str_start+=strlen(&name_buf[str_start])+1){
        //get the value for this attribute
        ssize_t size = (opt_mask.nibble&NO_DEREF) ? lgetxattr(file_name,&name_buf[str_start],NULL,0) : getxattr(file_name,&name_buf[str_start],NULL,0);
        char value_buf[size*4];
        status = (opt_mask.nibble&NO_DEREF) ? lgetxattr(file_name,&name_buf[str_start],value_buf,size*4) : getxattr(file_name,&name_buf[str_start],value_buf,size*4);
        if(status!=XATTR_OK){
          errnoExit((opt_mask.nibble&NO_DEREF)? "lgetxattr()" : "getxattr()");
        }
        //print it
        printf("%s=%s\n",&name_buf[str_start],value_buf);
      }
  }
  else{
    errExit("%s\n","unimplemented options");
  }



  exit(EXIT_SUCCESS);
}
