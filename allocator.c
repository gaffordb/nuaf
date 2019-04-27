#define _GNU_SOURCE

#include <asm/unistd.h>
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>

#include "freelist.h"

#define DATA_SIZE 10000000000
#define LARGE_OBJ_VADDR_START 0xFFFF000000

/* obtained via /proc/sys/vm/mmap_min_addr */
#define MMAP_MIN_ADDR 65536
#define MMAP_MAX_ADDR CONFIG_PAGE_OFFSET

#define OBJ_HEADER sizeof(int8_t)
#define LARGE_OBJ_START_MAGIC 0xC001Be9

/* For use in actual malloc/free calls */
extern void* __libc_malloc(size_t);
extern void __libc_free(void*);

int data_fd = 0;
size_t high_vaddr = MMAP_MIN_ADDR;
size_t large_obj_next_page = LARGE_OBJ_VADDR_START;

pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_big_obj_m = PTHREAD_MUTEX_INITIALIZER;

void sigsegv_handler(int signal, siginfo_t* info, void* ctx) {
  printf("Got SIGSEGV at address: 0x%lx\n", (long)info->si_addr);
  exit(EXIT_FAILURE);
}

void __attribute__((destructor)) destroy_mem(void) { close(data_fd); }
void __attribute__((constructor)) init_mem(void) {
  if(data_fd != 0) { return; } // If already called
  
  // Make a sigaction struct to hold our signal handler information
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = sigsegv_handler;
  sa.sa_flags = SA_SIGINFO;

  // Set the signal handler, checking for errors
  if (sigaction(SIGSEGV, &sa, NULL) != 0) {
    perror("sigaction failed");
    exit(2);
  }

  errno = 0;

  /* Make backing data file */
  data_fd = open("./.my_data", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  if (data_fd == -1) {
    perror("open");
    exit(1);
  }

  /* Grow data file to the appropriate size */
  if (ftruncate(data_fd, DATA_SIZE)) {
    perror("ftruncate");
  }

  freelist_init(data_fd);
}

/* For objects larger than 1024 bytes -- each object given some N*PAGE_SIZE bytes */
void* xxmalloc_big(size_t size) {
  /* Allocate enough space for some metadata */
  size += sizeof(size_t) + 8;

  size_t num_pages = (size / PAGE_SIZE) + 1;

  pthread_mutex_lock(&g_big_obj_m);
  /* 
   * Make shadow starting at LARGE_OBJ_VADDR_START, and going up -- should use MAP_FIXED_NOREPLACE if kernel >= 4.17 -- This can clobber existing mappings!
   * Note: MAP_ANONYMOUS is okay here because we're not really even using shadow mappings... 
*/
  void* shadow = mmap((void*)large_obj_next_page,
                      num_pages * PAGE_SIZE, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, 0, 0);

  if(shadow == MAP_FAILED || shadow != (void*)large_obj_next_page) {
    perror("mmap failed");
    fprintf(stderr, "data_fd: %d\n", data_fd);
    exit(1);
  }

  /* Update next available vaddr for big objects */
  large_obj_next_page += PAGE_SIZE * num_pages; 

  pthread_mutex_unlock(&g_big_obj_m);
    
  /* So we know where big objs start */
  *(size_t*)shadow = (size_t)LARGE_OBJ_START_MAGIC;

  /* So we know how big the big obj is */
  *(size_t*)(shadow+sizeof(size_t)) = num_pages;
  
  //  fprintf(stderr, "allocated big obj %u @ %p\n", size, shadow);

  /* Return usable pointer (after object metadata)*/
  return shadow+sizeof(size_t)*2;
}

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
void* xxmalloc(size_t size) {
  /* For mallocs called before constructor, call constructor first */
  if (!data_fd) {
    init_mem();
  }

  // handle large object
  if (size > LARGEST_BLOCK_SIZE - OBJ_HEADER) {
    return xxmalloc_big(size);
  }

  /* Allocate enough space for some metadata */
  size += OBJ_HEADER;

  pthread_mutex_lock(&g_m);

  void* obj_vaddr = freelist_pop(size, &high_vaddr, data_fd);

  pthread_mutex_unlock(&g_m);

  return obj_vaddr;
}

size_t xxmalloc_usable_size(void* ptr);

/**
 * Free space occupied by a heap object.
 * \param ptr   A pointer somewhere inside the object that is being freed
 */
void xxfree(void* ptr) {
  /* If the backing file has not been created (i.e., constructor has not been called */
  if (!data_fd) {
    init_mem();  // Call constructor...
  }
  
  if (ptr == NULL) {
    return;
  }

  /* Determine object size */
  size_t obj_size = xxmalloc_usable_size(ptr);

  if (obj_size >= PAGE_SIZE) {
    munmap((void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE), obj_size);
  } else {


    /* Get a fresh virtual page for the same canonical object */
    void* new_vpage =
        mremap((void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE), PAGE_SIZE,
               PAGE_SIZE, MREMAP_FIXED | MREMAP_MAYMOVE, high_vaddr);

    if (new_vpage == MAP_FAILED) {
      perror("mremap");
      exit(1);
    }

    high_vaddr += PAGE_SIZE;

    void* new_obj_vaddr = new_vpage + ((intptr_t) new_vpage & PAGE_SIZE);
    freelist_push(new_vpage);
  }

  return;
}

/**
 * Get the available size of an allocated object
 * \param ptr   A pointer somewhere inside the allocated object
 * \returns     The number of bytes available for use in this object
 */
size_t xxmalloc_usable_size(void* ptr) {
  if((intptr_t)ptr >= LARGE_OBJ_VADDR_START) {
    /* Keep going down pages until we find the beginning of the object */
    while (*(intptr_t*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE) !=
           LARGE_OBJ_START_MAGIC) {
      ptr -= PAGE_SIZE;
    }
    /* number of objects is stored here... */
    return *(size_t*)(ROUND_DOWN((intptr_t)ptr,PAGE_SIZE) \
                      + sizeof(intptr_t)) * PAGE_SIZE;
    
  } else {
    return 0;
  }
}
