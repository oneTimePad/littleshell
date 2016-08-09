#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "errors.h"
#include "utils.h"


/**
* safely determine if ruid has access to this file
* file_name: file to check
* returns: status
**/
_BOOL safe_access(const char* file_name, int flags){
  //determine if it exists, by attempting to open for reading
  uid_t saved_euid = geteuid();
  uid_t saved_egid = getegid();
  uid_t ruid = getuid();
  uid_t rgid = getgid();
  seteuid(ruid);
  setegid(rgid);

  errno =0;
  int fd;
  //can we open for reading using out ruid/rgid?
  if((fd=open(file_name,(flags&A_WOK) ? ((flags&A_ROK) ? O_RDWR : O_WRONLY ):O_RDONLY))==-1){
    seteuid(saved_euid);
    setegid(saved_egid);
    return FALSE;
  }

  struct stat stats;
  //use fstat to avoid race condition that would occur with stat
  if(fstat(fd,&stats)==-1)
    return FALSE;
  if(flags&A_XOK){
    //can we execute it with this ruid?
    if(ruid==stats.st_uid&&stats.st_mode&S_IXUSR){
        if(close(fd)==-1){
          seteuid(saved_euid);
          setegid(saved_egid);
          return FALSE;
        }
        return TRUE;
      }
      //can't, can we execute it with this rgid?
      else if(rgid==stats.st_gid&&stats.st_mode&S_IXGRP){
        if(close(fd)==-1){
          seteuid(saved_euid);
          setegid(saved_egid);
          return FALSE;
        }
        return TRUE;
      }
  }

  if(close(fd)==-1){
    seteuid(saved_euid);
    setegid(saved_egid);
    return FALSE;
  }
  return FALSE; //nope we can't we this file
}



//#include "my_env.h"

/**
* get current environ size
**/
/*
static ssize_t getenvsize(void){
  ssize_t size = 0;
  char** my_environ = environ;
  for(;*my_environ!=NULL;my_environ++)
    size++;

  return size;

}*/


/**
* get current environ size
**/
/*
static ssize_t getenvsize(void){
  ssize_t size = 0;
  char** my_environ = environ;
  for(;*my_environ!=NULL;my_environ++)
    size++;

  return size;

}*/


/**
* removes all allocated strings
* returns: status
**/
/*
static short free_env(void){
  if(environ == NULL) return -1;
  char** my_environ = environ;
  for(;*my_environ!=NULL;my_environ++)
    if(strncmp(*my_environ+strlen(*my_environ)+1,MALLOC_MAGIC_STRING,MAGIC_STRING_LEN)==0)
      free(*my_environ);

  return 0;
}*/



/**
* WARNING: not thread safe!
* implements setenv, makes copy of name,value and inserts if overwrite=1&exists else nothing if exists
* appends magic string to end of addition, so it knows to free it later
* name: name of environment var name to insert
* value: value to insert for name
* overwrite: 0 or 1 to identify if we should overwrite existing env var
* still leaks the if we have to realloc whole environment list
* also doesn't make a good reuse of memory, just keeps mallocing and freeing
* returns: success status
**/
/*
int safe_setenv(const char *name,const char *value, int overwrite){
    if(environ == NULL) return -1;
    if(overwrite!=0 && overwrite!=1) return -1; //only accept false or true
    if(name==NULL || strncmp(name,"\0",strlen(name))==0 || strncmp(name,"=",strlen(name))==0){
        errno = EINVAL;
       return -1;
     }

    int name_len = strlen(name);
    if(name_len-1>=MAX_ENV_NAME) return -1;
    int value_len = strlen(value);
    if(value_len-1>=MAX_VAL_NAME) return -1;

    char* putenv_cpy = (char*)malloc(name_len+value_len+1+MAGIC_STRING_LEN+1); //allocate for name/value, 2 nulls, and magic
    strncpy(putenv_cpy,name,name_len+1); //null term
    strncat(putenv_cpy,"=",1); //append "="
    strncat(putenv_cpy,value,value_len); //add the value
    strncat(putenv_cpy+strlen(putenv_cpy)+1,MALLOC_MAGIC_STRING,MAGIC_STRING_LEN);//add a magic string passed the null term
    char* getenv_result = getenv(name);

    char** my_environ = environ;
    if(getenv_result!=NULL && overwrite){
        for(;*my_environ!=NULL; my_environ++)
          if(strncmp(*my_environ,name,name_len)==0)
            break;


        //to avoid invalid memory access (since we are inspecting possibly invalid memory)
        //calloc enough and copy the name=value pair
        //this possibly could cause a SIGSEGV...it has yet to do so
        size_t new_entry_size = strlen(*my_environ)+1+MAGIC_STRING_LEN+1;
        char* entry = (char*)calloc(sizeof(char),new_entry_size);
        char* orig_entry= entry;
        if(entry == NULL)
          return -1;
        memcpy(entry,*my_environ,new_entry_size);

        size_t entry_len = strlen(entry);
        size_t putenv_len = strlen(putenv_cpy);
        _BOOL is_big_enough = (strlen(*my_environ)+1 < strlen(putenv_cpy)+1) ? FALSE : TRUE;


        entry = entry+strlen(*my_environ)+1; //access beggining of where the MAGIC_STRING would be

        int  no_found_magic = strncmp(entry,MALLOC_MAGIC_STRING,MAGIC_STRING_LEN+1);
        if(!no_found_magic&&!is_big_enough){ //if magic string found and current is not big enough to hold new
          free(*my_environ);
          *my_environ = putenv_cpy; //add the new entry
        }
        else if(no_found_magic&&!is_big_enough){ //if no magic string and is not big enough
          *my_environ = putenv_cpy;
          free(putenv_cpy);
        }
        else if(is_big_enough){ //if is big enough
          size_t left_over = (entry_len-putenv_len);
          strcpy(*my_environ,putenv_cpy);
          memset(*(my_environ)+putenv_len+1,'H',left_over);
          free(putenv_cpy);
        }

        free(orig_entry);
    }
    else if(getenv_result==NULL){

      char** new_environ;
      ssize_t env_size = getenvsize();
      new_environ = (char**)malloc(env_size+sizeof(char*)*2);
      if(new_environ == NULL)
        return -1;

      char** environ_cpy=environ;
      char** new_environ_cpy= new_environ;
      for(;*environ_cpy!=NULL;environ_cpy++){ //move all entries into new space
        *new_environ_cpy = *environ_cpy;
        new_environ_cpy++;
      }
      *new_environ_cpy = putenv_cpy; //add the new entry at the end
      *(new_environ_cpy+1) = NULL; //add null

      environ = new_environ;

    }
    else
        return -1;

    return (getenv(name)!=NULL) ? 0 : -1;

}
*/
/**
* unsets an environment variable and frees it allocated by setenv
* partially fixes memory leak problem with setenv in linux (roundabout way)
* name: name to remove in name=value pair
* when name = "=", all allocated strings are removed
* returns: status
**/
/*
int safe_unsetenv(const char *name){
  if(environ == NULL) return -1;
  if(name==NULL) return -1;

  if(strncmp(name,"=",strlen(name))==0)
    return (int)free_env();



  char** my_environ = environ;
  for(;*my_environ!=NULL;my_environ++){
    //if its the one we are looking to remove
    if(!strncmp(*my_environ,name,strlen(name)) && (*my_environ)[strlen(name)]=='='){
      char** loc = my_environ;
      //look for magic string
      if(strncmp(*my_environ+strlen(*my_environ)+1,MALLOC_MAGIC_STRING,MAGIC_STRING_LEN)==0)
        free(*my_environ); // free it
        //move all entries up one
      do // neat trick from glibc implementation
        loc[0] = loc[1];
      while(*loc++);
    }
  }

  return (getenv(name)==NULL)? 0: -1;

}*/
