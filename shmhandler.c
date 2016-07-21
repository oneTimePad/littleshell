#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "shmhandler.h"


/**
* initialize shared memory segment
* args: f_shm: location of shm, length: size of shm
**/
void* mminit(const char * f_shm, size_t length){
  //open of the fd for the shm seg
  int shm_fd;

  if(access(f_shm,F_OK)==-1){
    if((shm_fd =open(f_shm,O_CREAT| O_RDWR,0666))==-1){
        perror("open()");
        return NULL;
    }

    posix_fallocate(shm_fd,0,length);
    close(shm_fd);

  }

    if((shm_fd =open(f_shm,O_RDWR,0666))==-1){
        perror("open()");
        return NULL;
    }

  //attach the mem segment, and return ptr on success
  void* mem_attach = mmap(NULL,length,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,(off_t)0x0);
  if(mem_attach == MAP_FAILED){
      perror("mmap()");
      return NULL;
  }
  return  mem_attach;
}

void mmrelease(void* mem_ptr,size_t length){
    if(mem_ptr == NULL) return;
    munmap(mem_ptr,length);



}
