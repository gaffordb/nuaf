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
    g_flsts[i].high_canonical_addr = i * PAGE_SIZE + BYTE_ALIGNMENT;
  }
}

/* should be call when someone is freed
/* Add to the front of freelist */
void freelist_push(void* obj_vaddr) {
  int block_type = *((int*)(ROUND_DOWN(
      (intptr_t)obj_vaddr,
      PAGE_SIZE)));  // read the type metadata in the byte before vaddr

  *((void**)obj_vaddr) =
      g_flsts[block_type].top_of_stack;  // store the current top in the new real_vaddr
  g_flsts[block_type].top_of_stack = obj_vaddr;
}

/* Return the top of freelist */
void* freelist_pop(size_t obj_size, intptr_t* high_vaddr, int data_fd) {
  int8_t block_type =
      obj_size < MIN_BLOCK_SIZE ? 0 : (int)(log2(obj_size)) - MAGIC_NUMBER;
  void* obj_vaddr = NULL;
  if (g_flsts[block_type].top_of_stack == NULL) {  // freelist is empty
  
    void* beginning_vaddr =
        mmap((void*)(*high_vaddr), PAGE_SIZE, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED, data_fd,
             ROUND_DOWN(g_flsts[block_type].high_canonical_addr, PAGE_SIZE));
    
    if (beginning_vaddr == MAP_FAILED || beginning_vaddr == NULL) {
      perror("mmap failed");
      fprintf(stderr, "data_fd: %d\n", data_fd);
      fprintf(stderr, "attempted to map 1 page to %p\n", *high_vaddr);
      exit(1);
    }
    if(*(int*)beginning_vaddr != block_type) {
      *(int*)beginning_vaddr = block_type;
    }
    (*high_vaddr) += PAGE_SIZE;

    obj_vaddr =
        beginning_vaddr + (g_flsts[block_type].high_canonical_addr % PAGE_SIZE);

    g_flsts[block_type].high_canonical_addr += 1 << ((int)MAGIC_NUMBER + block_type);

    /* If we fill up a page, move on to the next one.
       Things to keep in mind:
       - This works bc objects are powers of 2
       - Size-segregated pages, cycles of length NUM_OBJECT_SIZES pages
    */
    if ((intptr_t)g_flsts[block_type].high_canonical_addr % PAGE_SIZE > (intptr_t)PAGE_SIZE - (1 << ((int)MAGIC_NUMBER + block_type))) {
      
      g_flsts[block_type].high_canonical_addr =
          ROUND_UP(g_flsts[block_type].high_canonical_addr, PAGE_SIZE)+ PAGE_SIZE * (NUM_BLOCK_TYPES - 1) + BYTE_ALIGNMENT;
    }
  } else {
    obj_vaddr = g_flsts[block_type].top_of_stack;
    g_flsts[block_type].top_of_stack = *((void**)obj_vaddr);
  }

  return obj_vaddr;
}
