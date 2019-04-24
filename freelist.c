#include "freelist.h"
#include <math.h>
#include <stdbool.h>
#include <sys/types.h>

/* Init freelist */
void freelist_init(int data_fd) {
  for (int i = 0; i < NUM_OBJECT_SIZES; i++) {
    freelist_t cur_freelist = g_flsts[i];
    cur_freelist.offset = -1;  // means there is no free space
    cur_freelist.current_page = i;
  }
}

/* should be call when someone is freed
/* Add to the front of freelist -- return true if add successful (i.e., freelist
not full) */
/* we may want to directly pass the size of the freed object to this function as
 * freelists is global */
bool freelist_push(size_t obj_size, void* vaddr, off_t canonical_addr) {
  int list_index = (int)log2(obj_size) - 3 < 0 ? 0 : (int)log2(size) - 3 < 0;
  freelist_t cur_freelist = g_flsts[list_index];
  cur_freelist.offset++;
  cur_freelist.buff[cur_freelist.offset].vaddr = vaddr;
  cur_freelist.buff[cur_freelist.offset].canonical_addr = canonical_addr;

  return true;  //??? do we need this boolean?
}

/* Take from the freelist and fill in provided mapping with data */
bool freelist_pop(size_t obj_size, mapping_t* mapping) {
  int list_index = (int)log2(obj_size) - 3 < 0 ? 0 : (int)log2(size) - 3 < 0;
  freelist_t cur_freelist = g_flsts[list_index];
  if (cur_freelist.offset == -1) {  // no free space
    // mmap(current_canonical_addr);
    if (mmap wrong) {
      perror();
      exit();
    }
    return true; //need?
  } else {
    *mapping = cur_freelist.buff[cur_freelist.offset];
    cur_freelist.offset--;
    return true; //need? 
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