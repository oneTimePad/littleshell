#ifndef _SHMHANDLER
#define _SHMHANDLER
#include <sys/mman.h>

void * mminit(const char * f_shm, size_t length);
void mmrelease(void* mem_ptr,size_t length);

#endif
