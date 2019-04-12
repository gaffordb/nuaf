#define _GNU_SOURCE

#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DATA_SIZE 1000000000
#define PAGE_SIZE 0x1000

extern void* __libc_malloc(size_t);

int data_fd;
size_t high_watermark;

void __attribute__((constructor)) init_mem(void) {
  errno = 0;
  data_fd = open("/tmp/data_for_me", O_CREAT|O_RDWR);
  ftruncate(data_fd, DATA_SIZE);
  high_watermark = 0;
  if(!errno) {
    perror("something?");
  }
}

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
void* xxmalloc(size_t size) {


  /* Call original malloc, but with enough space to include address for bookkeeping (as done in Dhurjati & Adve (2006), and Oscar) */
  //void* canonical = __libc_malloc(size+sizeof(void*));
  //  void* shadow_page = mremap(canonical, 0, size+sizeof(void*), flags); //somehow we will force this to only increase during one execution

  void* shadow = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, high_watermark, data_fd);
  if (shadow == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }
  
  high_watermark += PAGE_SIZE; // 1 obj per physical page
  
  // void* canonical_page = mmap(NULL, size + sizeof(void*), PROT_NONE, MAP_SHARED | MAP_ANONYMOUS);
  /* I guess fprintf doesn't have any internal malloc calls */ /* \>.</ */
  fprintf(stderr, "allocated %ul @ %p\n", size+sizeof(void*), shadow);
  
  // The first sizeof(ptr) will be use to store allocated; 
  // Some mremap work to create the shadow page 
  
  return (shadow + sizeof(void*));
}

size_t xxmalloc_usable_size(void* ptr);

/**
 * Free space occupied by a heap object.
 * \param ptr   A pointer somewhere inside the object that is being freed
 */
void xxfree(void* ptr) {
  munmap(ptr, xxmalloc_usable_size(ptr));
  // unmap the shadow page
  //  free(/* ptr_containing_the_address_in_the_canonical_page*/);
}

/*
 * Get the available size of an allocated object
 * \param ptr   A pointer somewhere inside the allocated object
 * \returns     The number of bytes available for use in this object
 */
size_t xxmalloc_usable_size(void* ptr) {
  // doesn't matter
  return PAGE_SIZE;
}

