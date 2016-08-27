#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "path.h"

char LPATH[] = "LPATH=./bin";

/**
* checks if file is in `path` WARINING: this doesn't check permission(only for existance!)
* if we check for permissions, users in shell won't be able to run SUID/SGID programs
* all we care to check here is in the file exists in some path entry
* file: file to check
* fpath: buffer to hold location of file
* size : size of fpath
* returns: status
**/
_BOOL inPath(char *file,char* fpath,size_t size){
  char full_path[PATH_LIM];
  char * path_var;

  #ifdef _GNU_SOURCE
  if((path_var=secure_getenv(PATH))==NULL)
    return FALSE;
  #else
  if((path_var=getenv(PATH))==NULL)
    return FALSE;
  #endif

  int cur_index =0;
  _BOOL found = FALSE;
  //search all files in path
  int start = 0;
  int index = 0;
  while(path_var[index++]!='\0'){
      if(path_var[index]==':' || path_var[index] =='\0'){

        strncpy(full_path,path_var+start,index-start);
        full_path[index-start] = '/';
        full_path[index-start+1]   = '\0';
        strcat(full_path,file);
        start = index+1;
        struct stat buf;
        if(stat(full_path,&buf)!=-1){
          strcpy(fpath,full_path);
          return TRUE;
        }
        else if(errno =  ENOENT)
          continue;
        else //unknown error
          return FALSE;
      }
  }




}
