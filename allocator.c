#define _GNU_SOURCE

#include <asm/unistd.h>
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

/*
static void* mmap2(void* addr, size_t length, int prot, int flags, int fd, off_t
pgoffset) { return syscall(__NR_mmap2, addr, length, prot, flags, fd, pgoffset);
}
*/
#define ROUND_UP(X, Y) ((X) % (Y) == 0 ? (X) : (X) + ((Y) - (X) % (Y)))
#define ROUND_DOWN(X, Y) ((X) % (Y) == 0 ? (X) : (X) - ((X) % (Y)))

#define MIN_SIZE 8
#define DATA_SIZE 0x1000000000  // Now it is a hex!
#define PAGE_SIZE 0x1000

/* obtained via /pro/sys/vm/mmap_min_addr */
#define MMAP_MIN_ADDR 65536

typedef struct obj_header {
  intptr_t canonical_addr;
} obj_header_t;

#define OBJ_HEADER_SIZE sizeof(obj_header_t)

/* For use in actual malloc/free calls */
extern void* __libc_malloc(size_t);
extern void __libc_free(void*);

int data_fd = 0;
size_t physical_page = 0;
size_t next_page = MMAP_MIN_ADDR;

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

  // Create a physical memory
  data_fd = open("./.my_data", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

  if (data_fd == -1) {
    perror("open");
    exit(1);
  }

  if (ftruncate(data_fd, DATA_SIZE)) {
    perror("ftruncate");
  }

  // Set the high watermark
  // physical_page = PAGE_SIZE;

  // size_t next_page = MMAP_MIN_ADDR;

  // // Use the first page to hold the free physical page list:
  // long long* first_page =
  //     mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
  //          MAP_PRIVATE, data_fd, 0);

  // // Store all the free pages offset in a list
  // long page_amount = DATA_SIZE / PAGE_SIZE;
  // long long i = PAGE_SIZE;
  // long j = 0;
  // for (; i < DATA_SIZE; i += PAGE_SIZE) {
  //   printf("lalala, %ld\n", j);
  //   first_page[j] = i;
  //   j++;
  // }
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
  if (size > PAGE_SIZE - OBJ_HEADER_SIZE) {
    printf(
        "Large objects don't work right now, handle special case later.^_^\n");
    return NULL;
  }

  /* Allocate enough space for some metadata */
  size += OBJ_HEADER_SIZE;
  size_t usable_space = PAGE_SIZE - physical_page % PAGE_SIZE;
  size_t num_pages = (size / PAGE_SIZE) + 1;

  if (size > usable_space) {  // The current page is not enough, we need a new
                              // page for the object
    // TODO: Mark the unused space as usable.

    // Move the physical_page to the next physical page
    physical_page += ROUND_UP(physical_page, PAGE_SIZE);
  } 

  // _page is the virtual _page page allocated for the object.
  /* Make _page starting at MMAP_MIN_ADDR, and going up according to
   * physical_page. mmap2 used so we can index further into the underlying
   * buffer (give offset in terms of num_pages rather than num_bytes*/
  intptr_t shadow_page = (intptr_t)mmap((void*)next_page, num_pages * PAGE_SIZE,
                                   PROT_READ | PROT_WRITE, MAP_PRIVATE, data_fd,
                                   ROUND_DOWN(physical_page, PAGE_SIZE));

  if (_page == (intptr_t)MAP_FAILED) {
    perror("mmap failed");
    fprintf(stderr, "data_fd: %d\n", data_fd);
    exit(1);
  }

  // the next_page should be assigned to the shawdow page
  assert(_page == (intptr_t)next_page);

  // offset is where the object should be stored inside the page (_page or
  // physical)
  /* Calculate offset into virtual page that corresponds to physical data */
  unsigned int offset = (physical_page % PAGE_SIZE) + OBJ_HEADER_SIZE;

  /*
     Store canonical address in obj for future reuse.
     Note: physical_page is the canonical address
  */
  (*(obj_header_t*)(_page + offset - OBJ_HEADER_SIZE)).canonical_addr = physical_page;

  fprintf(stderr,
          "allocated %x @ virtual page: %p, physical page: %p, offset=%x\n",
          size, _page, physical_page, offset);

  physical_page += ROUND_UP(size, MIN_SIZE);
  next_page += PAGE_SIZE * num_pages;

  return (void*)(_page + offset);
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
  // ptr -= OBJ_HEADER;
  size_t obj_size = PAGE_SIZE;  // xxmalloc_usable_size(ptr);

  /* unmap the _page page */
  if (munmap((void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE), obj_size)) {
    fprintf(stderr, "ptr: %p, obj_size: %zu\n",
            (void*)ROUND_DOWN((intptr_t)ptr, PAGE_SIZE), obj_size);
    perror("munmap");
  }

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

/*
void* realloc(void* ptr, size_t size) {
  void* new_mem = xxmalloc(size);
  bcopy(ptr, new_mem, size);
  xxfree(ptr);
  return xxmalloc(size);
}
*/
