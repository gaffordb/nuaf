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

#include "freelist.h"

#define MIN_SIZE 8
#define DATA_SIZE 1000000000

/* obtained via /pro/sys/vm/mmap_min_addr */
#define MMAP_MIN_ADDR 65536

typedef struct obj_header {
  intptr_t canonical_addr;
} obj_header_t;

#define OBJ_HEADER sizeof(obj_header_t)

/* For use in actual malloc/free calls */
extern void* __libc_malloc(size_t);
extern void __libc_free(void*);

int data_fd = 0;
size_t canonical_addr = 0;
size_t next_page = MMAP_MIN_ADDR;
pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;

void sigsegv_handler(int signal, siginfo_t* info, void* ctx) {
  printf("Got SIGSEGV at address: 0x%lx\n", (long)info->si_addr);
  exit(EXIT_FAILURE);
}

void __attribute__((destructor)) destroy_mem(void) { close(data_fd); }
void __attribute__((constructor)) init_mem(void) {
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

  data_fd = open("./.my_data", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  
  if (data_fd == -1) {
    perror("open");
    exit(1);
  }

  if (ftruncate(data_fd, DATA_SIZE)) {
    perror("ftruncate");
  }

  freelist_init(data_fd);
    
  canonical_addr = 0;

  size_t next_page = MMAP_MIN_ADDR;
}

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
void* xxmalloc(size_t size) {
  /* For mallocs called before constructor, use glibc implementation */
  if (!data_fd) {
    return __libc_malloc(size);
  }
  if (size > PAGE_SIZE - OBJ_HEADER || size > 1024 - OBJ_HEADER) {
    fprintf(
        stderr,
        "Large objects don't work right now, handle special case later. ^_^\n");
    errno = ENOTSUP;
    return NULL;
  }

  /* Allocate enough space for some metadata */
  size += OBJ_HEADER;

  pthread_mutex_lock(&g_m);

  mapping_t m;
  freelist_pop(size, &m, &next_page, data_fd);

  /* Calculate offset into virtual page that corresponds to physical data */
  unsigned int offset = (m.canonical_addr % PAGE_SIZE) + OBJ_HEADER;

  /*
     Store canonical address in obj for future reuse.
     Note: canonical_addr is the canonical address
  */
  (*(obj_header_t*)(m.vaddr + offset - OBJ_HEADER)).canonical_addr =
      m.canonical_addr;

  fprintf(stderr,
          "allocated %x @ virtual page: %p, physical page: %p, offset=%x\n",
          size, m.vaddr, m.canonical_addr, offset);

  pthread_mutex_unlock(&g_m);

  return (void*)(m.vaddr + OBJ_HEADER);
}

size_t xxmalloc_usable_size(void* ptr);

/**
 * Free space occupied by a heap object.
 * \param ptr   A pointer somewhere inside the object that is being freed
 */
void xxfree(void* ptr) {
  if (!data_fd) {
    __libc_free(ptr);
    return;
  }
  if (ptr == NULL) {
    return;
  }

  //  size_t obj_size = PAGE_SIZE;  // xxmalloc_usable_size(ptr); for large pages

  /* Retrieve canonical address that was stored in object metadata */
  off_t canonical_addr = *(off_t*)((intptr_t)ptr-sizeof(intptr_t));
  
  /* Get a fresh virtual page for the same canonical object */
  void* fresh_vaddr = mremap((void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE),
                              PAGE_SIZE, PAGE_SIZE,
                              MREMAP_FIXED | MREMAP_MAYMOVE, next_page);
  
  assert((intptr_t)fresh_vaddr == next_page);
  next_page += PAGE_SIZE;

  /* Determine object size */
  size_t obj_size = get_obj_size_by_addr(canonical_addr);
  
  freelist_push(obj_size, fresh_vaddr, canonical_addr);
  fprintf(stderr, "Unmapped %p to %p \n",
          (void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE),
          (void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE) + obj_size);
}

/**
 * Get the available size of an allocated object
 * \param ptr   A pointer somewhere inside the allocated object
 * \returns     The number of bytes available for use in this object
 */
size_t xxmalloc_usable_size(void* ptr) {
  size_t num_pages = (*(size_t*)ptr);
  //  fprintf(stderr, "Actual size is: %zu\n", num_pages);
  return PAGE_SIZE * num_pages;
}

