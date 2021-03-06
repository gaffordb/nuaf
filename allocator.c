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

#define DATA_SIZE 0x10000000000
#define LARGE_OBJ_VADDR_START 0xFFFF000000

/* Start of kernel-owned memory */
#define MMAP_MAX_ADDR CONFIG_PAGE_OFFSET

/* Indicates start of large objects */
#define LARGE_OBJ_START_MAGIC 0xC001Be9

/* For use in actual malloc/free calls */
extern void* __libc_malloc(size_t);
extern void __libc_free(void*);

size_t num_mappings;
int data_fd = 0;
size_t high_vaddr = MMAP_MIN_ADDR;
size_t large_obj_next_page = LARGE_OBJ_VADDR_START;

pid_t par_ps = 0;

pthread_mutex_t g_m = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t g_big_obj_m = PTHREAD_MUTEX_INITIALIZER;

void sigsegv_handler(int signal, siginfo_t* info, void* ctx) {
  printf("Got SIGSEGV at address: 0x%lx\n", (long)info->si_addr);
  // Uncomment below for printing program mappings upon segfault
  // int fd = open("/proc/self/maps", O_RDONLY); 
  // char buf[1024];
  // int buflen;
  // while ((buflen = read(fd, buf, 1024)) > 0) {
  //   write(1, buf, buflen);
  // }
  // close(fd);
  exit(EXIT_FAILURE);
}

void __attribute__((destructor)) destroy_mem(void) { close(data_fd); }
void __attribute__((constructor)) init_mem(void) {
  
  /* If already called in this process, or uncalled in new child process */
  if (par_ps == getpid()) {
    return;
  }

  data_fd = 0;
  par_ps = getpid();
  high_vaddr = MMAP_MIN_ADDR;
  large_obj_next_page = LARGE_OBJ_VADDR_START;
  num_mappings = 0;

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

  /* Make backing data file */
  char stemp[23] = "/tmp/nuaf_data_XXXXXX";
  data_fd = mkstemp(stemp);

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

/* For objects larger than 1024 bytes -- each object given some N*PAGE_SIZE
 * bytes */
void* xxmalloc_big(size_t size) {
  /* Allocate enough space for some metadata */
  size += sizeof(size_t) + 8;

  size_t num_pages = (size / PAGE_SIZE) + 1;

  pthread_mutex_lock(&g_big_obj_m);
  /*
   * Make shadow starting at LARGE_OBJ_VADDR_START, and going up -- should use
   * MAP_FIXED_NOREPLACE if kernel >= 4.17 -- This can clobber existing
   * mappings! Note: MAP_ANONYMOUS is okay here because we're not really even
   * using shadow mappings...
   */
  void* shadow = mmap((void*)large_obj_next_page, num_pages * PAGE_SIZE,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, 0, 0);

  if (shadow == MAP_FAILED || shadow != (void*)large_obj_next_page) {
    perror("mmap failed");
    fprintf(stderr, "data_fd: %d, the error code is %d\n", data_fd, errno);
    exit(1);
  }
  num_mappings++;

  /* Update next available vaddr for big objects */
  large_obj_next_page += PAGE_SIZE * num_pages;

  pthread_mutex_unlock(&g_big_obj_m);

  /* So we know where big objs start */
  *(size_t*)shadow = (size_t)LARGE_OBJ_START_MAGIC;

  /* So we know how big the big obj is */
  *(size_t*)(shadow + sizeof(size_t)) = num_pages;

  // fprintf(stderr, "allocated big obj %u @ %p\n", size, shadow);

  /* Return usable pointer (after object metadata)*/
  return shadow + sizeof(size_t) * 2;
}

/**
 * Allocate space on the heap.
 * \param size  The minimium number of bytes that must be allocated
 * \returns     A pointer to the beginning of the allocated space.
 *              This function may return NULL when an error occurs.
 */
void* xxmalloc(size_t size) {
  // fprintf(stderr,"mallocing!\n");
  /* For mallocs called before constructor, call constructor first */
  if (!data_fd) {
    init_mem();
  }

  /* If we can only fit 1 per page, use allocator for big obj */
  if (size > LARGEST_BLOCK_SIZE) {
    return xxmalloc_big(size);
  }

  pthread_mutex_lock(&g_m);

  /* Get next object (actual address to be given to user) */
  void* obj_vaddr = freelist_pop(size, &high_vaddr, data_fd);

  pthread_mutex_unlock(&g_m);

  //  fprintf(stderr, "num_mappings: %zu\n", num_mappings++);
  return obj_vaddr;
}

size_t xxmalloc_usable_size(void* ptr);

/* Point to start of the object -- obj size necessary for small objs
 *  Intended to deal with interior pointers */
void get_obj_start(void** ptr, size_t obj_size) {
  if ((intptr_t)*ptr >= LARGE_OBJ_VADDR_START) {
    /* Keep going down pages until we find the beginning of the object */
    while (*(intptr_t*)ROUND_DOWN((intptr_t)*ptr, PAGE_SIZE) !=
           LARGE_OBJ_START_MAGIC) {
      *ptr -= PAGE_SIZE;
    }
    /* We're on the right page, just round down */
    *ptr = (void*)ROUND_DOWN((intptr_t)*ptr, PAGE_SIZE);
  } else /* Small objs */ {
    /* Round down to the nearest multiple of obj_size */
    *ptr = (void*)(ROUND_DOWN((intptr_t)*ptr - BYTE_ALIGNMENT, obj_size) +
                   BYTE_ALIGNMENT);
  }
}

/**
 * Free space occupied by a heap object.
 * \param ptr   A pointer somewhere inside the object that is being freed
 */
void xxfree(void* ptr) {
  /* If the backing file has not been created
   * (i.e., constructor has not been called) */
  if (!data_fd) {
    init_mem();
  }

  if (ptr == NULL) {
    return;
  }

  // fprintf(stderr, "num_mappings: %zu\n", num_mappings++);

  /* Determine object size */
  size_t obj_size = xxmalloc_usable_size(ptr);

  /* Get start of object */
  get_obj_start(&ptr, obj_size);

  if ((intptr_t)ptr >= LARGE_OBJ_VADDR_START) {
    /* Unmap object (NOTE: not reusing large objects) */
    munmap((void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE),
           ROUND_UP(obj_size, PAGE_SIZE));
    num_mappings--;
    return;
  } else {
    pthread_mutex_lock(&g_m);

    /* Get a fresh virtual page for the same canonical object */
    int i = 1;
    void* new_vpage;
    while ((new_vpage = mremap((void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE),
                               PAGE_SIZE * i++, PAGE_SIZE,
                               MREMAP_FIXED | MREMAP_MAYMOVE, high_vaddr)) ==
               MAP_FAILED &&
           i < 100) {
      perror("mremap");
      fprintf(stderr, "prev_addr: %p, new_addr: %p\n",
              (void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE), (void*)high_vaddr);
      fprintf(stderr, "trying to mremap with %d pages...\n", i);
    }

    /* mremap failed for some other reason */
    if (i == 100) {
      fprintf(stderr, "mmap gave too many pages for some reason...\n");

      perror("mremap");
      exit(1);
    }

    high_vaddr += PAGE_SIZE;

    void* new_obj_vaddr = new_vpage + ((intptr_t)ptr % PAGE_SIZE);

    freelist_push(new_obj_vaddr);

    pthread_mutex_unlock(&g_m);
  }
}

/**
 * Get the available size of an allocated object
 * \param ptr   A pointer somewhere inside the allocated object
 * \returns     The number of bytes available for use in this object
 */
size_t xxmalloc_usable_size(void* ptr) {
  if (ptr == NULL) {
    return 0;
  }

  /* Large objects */
  if ((intptr_t)ptr >= LARGE_OBJ_VADDR_START) {
    /* Keep going down pages until we find the beginning of the object */
    while (*(intptr_t*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE) !=
           LARGE_OBJ_START_MAGIC) {
      ptr -= PAGE_SIZE;
    }
    return *(size_t*)(ROUND_DOWN((intptr_t)ptr, PAGE_SIZE) + sizeof(size_t)) *
               PAGE_SIZE -
           16;  // -16 bc header is not usable
  }

  /* Small objects */
  else {
    return 1 << (*(int*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE) + MAGIC_NUMBER);
  }
  return 0;
}

void* xxmalloc_lock() {
  pthread_mutex_lock(&g_m);
  pthread_mutex_lock(&g_big_obj_m);
}

void* xxmalloc_unlock() {
  pthread_mutex_unlock(&g_m);
  pthread_mutex_unlock(&g_big_obj_m);
}
