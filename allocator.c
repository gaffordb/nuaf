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

/* obtained via /pro/sys/vm/mmap_min_addr */
#define MMAP_MIN_ADDR 65536

/* For use in actual malloc/free calls */
extern void* __libc_malloc(size_t);
extern void __libc_free(void*);

int data_fd = 0;
size_t high_watermark = 0;

void __attribute__((destructor)) destroy_mem(void) {
  close(data_fd);
}
void __attribute__((constructor)) init_mem(void) {
  errno = 0;

  data_fd = open("./.my_data", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  if(data_fd == -1) {
    perror("open");
    exit(1);
  }

  if(ftruncate(data_fd, DATA_SIZE)) {
    perror("ftruncate");
  }

  high_watermark = 0;

}

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
void* xxmalloc(size_t size) {

  /* For mallocs called before constructor, use glibc implementation */
  if(!data_fd) {
    return __libc_malloc(size);
  }

  size_t page_number = (size / PAGE_SIZE) + 1;

  /* Make shadow starting at MMAP_MIN_ADDR, and going up according to high_watermark */
  void* shadow = mmap((void*)MMAP_MIN_ADDR+high_watermark, page_number *  PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, data_fd, high_watermark);
  if (shadow == MAP_FAILED) {
    perror("mmap failed");
    fprintf(stderr, "data_fd: %d\n", data_fd);
    exit(1);
  }
  
  high_watermark += PAGE_SIZE * page_number; // 1 obj per physical page
  
  // fprintf(stderr, "allocated %u @ %p\n", size, shadow);
  
  return shadow;
}

size_t xxmalloc_usable_size(void* ptr);

/**
 * Free space occupied by a heap object.
 * \param ptr   A pointer somewhere inside the object that is being freed
 */
void xxfree(void* ptr) {
  if(!data_fd) {
    __libc_free(ptr);
    return;
  }
  
  /* unmap the shadow page */
  if(munmap(ptr, xxmalloc_usable_size(ptr))) {
    perror("munmap");
  }
  
  fprintf(stderr, "Unmapped ptr: %p\n", ptr);
}

/**
 * Get the available size of an allocated object
 * \param ptr   A pointer somewhere inside the allocated object
 * \returns     The number of bytes available for use in this object
 */
size_t xxmalloc_usable_size(void* ptr) {
  // TODO
  return PAGE_SIZE;
}

