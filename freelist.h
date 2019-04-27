#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <stdint.h>

/* obtained via /pro/sys/vm/mmap_min_addr */
#define MMAP_MIN_ADDR 65536

#define ROUND_UP(X, Y) ((X) % (Y) == 0 ? (X) : (X) + ((Y) - (X) % (Y)))
#define ROUND_DOWN(X, Y) ((X) % (Y) == 0 ? (X) : (X) - ((X) % (Y)))

/*
   Obj sizes. Over 2^10 byte objects will be stored as large objects (1 per
   page) Two pages worth of freelist for each
*/
#define PAGE_SIZE 4096

#define MAGIC_NUMBER  3
#define MIN_BLOCK_SIZE  (1 << MAGIC_NUMBER)
#define LARGEST_BLOCK_SIZE  (PAGE_SIZE >> 1)
#define NUM_BLOCK_TYPES (12 - MAGIC_NUMBER)  // need to be more programatic

// typedef struct mapping {
//   void* vaddr; //not the start of the page (offset built in)
//   int8_t block_type; //Maybe there is a way to just use one of it in the beginning of every page
// } mapping_t;

typedef struct freelist {
  void* top_of_stack;        // the address of the top available block
  off_t high_canonical_addr;  //highest addr will be used by this freelist, used for
} freelist_t;

/* Init freelist */
void freelist_init();

/* should be call when someone is freed
/* Add to the front of freelist */
void freelist_push(void* obj_vaddr);

/* Return the address of the object, not the pointer to the block type metadata*/
void* freelist_pop(size_t obj_size, intptr_t* high_vaddr,
                  int data_fd);

/* NOTE: 'correct' free list determined by either obj_size, or by canonical
 * address */
// /* Take from correct free list, and put @ mapping */
// void get_mapping(size_t obj_size, mapping_t* mapping);

// /* Put into correct free list -- return true if successful (i.e., correct
//  * freelist not full) */
// bool put_mapping(void* vaddr, off_t canonical_addr);

// size_t get_obj_size_by_header(off_t canonical_addr);
