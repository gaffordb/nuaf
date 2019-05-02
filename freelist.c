#include "freelist.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>

/* Array of freelists */
freelist_t g_flsts[NUM_BLOCK_TYPES];

// const unsigned short g_obj_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024,
// 2048};

/* Init freelist */
void freelist_init() {
  for (int i = 0; i < NUM_BLOCK_TYPES; i++) {
    g_flsts[i].top_of_stack = NULL;  // means there is no free space
    g_flsts[i].high_canonical_addr = i * PAGE_SIZE;
  }
}

/* should be call when someone is freed
/* Add to the front of freelist */
void freelist_push(void* obj_vaddr) {
  void* real_vaddr = obj_vaddr - sizeof(int64_t);
  int8_t block_type =
      *((int8_t*)(obj_vaddr -
                  sizeof(int64_t)));  // read the type metadata in the byte before vaddr

  *((void**)real_vaddr) = g_flsts[block_type].top_of_stack; //store the current top in the new real_vaddr
  g_flsts[block_type].top_of_stack = real_vaddr;
}

/* Return the top of freelist */
void* freelist_pop(size_t real_size, intptr_t* high_vaddr, int data_fd) {
  int8_t block_type =
      real_size < MIN_BLOCK_SIZE ? 0 : (int)(log2(real_size)) - MAGIC_NUMBER;
  void* real_vaddr = NULL;
  if (g_flsts[block_type].top_of_stack == NULL) {  //freelist is empty

    // mapping->canonical_addr = g_flsts[list_index].high_canonical_addr;

    void * beginning_vaddr =
        mmap((void*)(*high_vaddr), PAGE_SIZE, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED, data_fd,
             ROUND_DOWN(g_flsts[block_type].high_canonical_addr, PAGE_SIZE));
    if (beginning_vaddr == MAP_FAILED || beginning_vaddr == NULL) {
      perror("mmap failed");
      fprintf(stderr, "data_fd: %d\n", data_fd);
      exit(1);
    }
    (*high_vaddr) += PAGE_SIZE;

    real_vaddr = beginning_vaddr + (g_flsts[block_type].high_canonical_addr % PAGE_SIZE);

    g_flsts[block_type].high_canonical_addr += 1 << block_type;

    /* If we fill up a page, move on to the next one.
       Things to keep in mind:
       - This works bc objects are powers of 2
       - Size-segregated pages, cycles of length NUM_OBJECT_SIZES pages
    */
    if ((intptr_t)g_flsts[block_type].high_canonical_addr % PAGE_SIZE == 0) {
      g_flsts[block_type].high_canonical_addr +=
          PAGE_SIZE * (NUM_BLOCK_TYPES - 1);
    }
  } else {
    real_vaddr = g_flsts[block_type].top_of_stack;
    g_flsts[block_type].top_of_stack = *((void **)real_vaddr);
  }

   *((int64_t*) real_vaddr) = block_type;
   return (real_vaddr + sizeof(int64_t));
}
