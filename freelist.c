#include "freelist.h"
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>

/* Array of freelists */
freelist_t g_flsts[NUM_OBJECT_SIZES];
const unsigned short g_obj_sizes[] = {16, 32, 64, 128, 256, 512, 1024};

/* Init freelist */
void freelist_init(int data_fd) {
  for (int i = 0; i < NUM_OBJECT_SIZES; i++) {
    g_flsts[i].offset = -1;  // means there is no free space
    g_flsts[i].high_canonical_addr = i*PAGE_SIZE;
  }
}

/* should be call when someone is freed
/* Add to the front of freelist -- return true if add successful (i.e., freelist
not full) */
/* we may want to directly pass the size of the freed object to this function as
 * freelists is global */
void freelist_push(size_t obj_size, void* vaddr, off_t canonical_addr) {
  int list_index = (int)log2(obj_size) - (int)log2(g_obj_sizes[0])< 0 ? 0 : (int)log2(obj_size) - (int)log2(g_obj_sizes[0]);
  freelist_t cur_freelist = g_flsts[list_index];
  if(cur_freelist.offset >= PAGE_SIZE * FREELIST_PAGE_SIZE / sizeof(mapping_t)) {
    return;
  }
  cur_freelist.offset++;
  cur_freelist.buff[cur_freelist.offset].vaddr = vaddr;
  cur_freelist.buff[cur_freelist.offset].canonical_addr = canonical_addr;
}

/* Take from the freelist and fill in provided mapping with data */
void freelist_pop(size_t obj_size, mapping_t* mapping, intptr_t* next_page,
                  int data_fd) {
  int list_index = (int)log2(obj_size) - (int)log2(g_obj_sizes[0]) < 0 ? 0 : (int)log2(obj_size) - (int)log2(g_obj_sizes[0]);
  freelist_t cur_freelist = g_flsts[list_index];
  if (cur_freelist.offset == -1) {  // no free space == freelist is empty

    mapping->vaddr =
        mmap((void*)(*next_page), PAGE_SIZE, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED, data_fd,
             ROUND_DOWN(cur_freelist.high_canonical_addr, PAGE_SIZE));
    if (mapping->vaddr == MAP_FAILED || mapping->vaddr == NULL) {
      perror("mmap failed");
      fprintf(stderr, "data_fd: %d\n", data_fd);
      exit(1);
    }
    (*next_page) += PAGE_SIZE;

    cur_freelist.high_canonical_addr += obj_size;

    /* If we fill up a page, move on to the next one.
       Things to keep in mind:
       - This works bc objects are powers of 2
       - Size-segregated pages, cycles of length NUM_OBJECT_SIZES pages 
    */
    if (cur_freelist.high_canonical_addr % PAGE_SIZE ==  0) {
      cur_freelist.high_canonical_addr += PAGE_SIZE*(NUM_OBJECT_SIZES-1);
    }

  } else {
    *mapping = cur_freelist.buff[cur_freelist.offset];
    cur_freelist.offset--;
  }
}

/* NOTE: 'correct' free list determined by either obj_size, or by canonical
 * address */
/* not sure about the meaning?
/* Take from correct free list, and put @ mapping */
void get_mapping(size_t obj_size, mapping_t* mapping) {}

/* Put into correct free list -- return true if successful (i.e., correct
 * freelist not full) */
bool put_mapping(void* vaddr, off_t canonical_addr) {}

/* Determine the canonical address from the object size */
size_t get_obj_size_by_addr(off_t canonical_addr) {
  return g_obj_sizes[ROUND_DOWN(canonical_addr, PAGE_SIZE) % NUM_OBJECT_SIZES];
}
