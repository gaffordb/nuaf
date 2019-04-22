#include <sys/types.h>
#include <stdbool.h>

/* obtained via /pro/sys/vm/mmap_min_addr */
#define MMAP_MIN_ADDR 65536

/* 
   Obj sizes. Over 2^10 byte objects will be stored as large objects (1 per page) 
   Two pages worth of freelist for each
*/
#define NUM_OBJECT_SIZES 8
const unsigned short g_obj_sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024};
#define FREELIST_SIZE 2

typedef struct mapping {
  void* vaddr;
  off_t canonical_addr;
} mapping_t;

typedef struct freelist {
  off_t offset;       //  
  off_t current_page; //
  mapping_t buff[PAGE_SIZE*FREELIST_SIZE/sizeof(mapping_t)];
} freelist_t;

/* Array of freelists */
freelist_t g_flsts[NUM_OBJECT_SIZES];

/* Init freelist */
void freelist_init(int data_fd);

/* Add to the front of freelist -- return true if add successful (i.e., freelist not full) */
bool freelist_add(freelist_t lst, void* vaddr, off_t canonical_addr);

/* Take from the freelist and fill in provided mapping with data */
void freelist_take(freelist_t lst, size_t obj_size, mapping_t* mapping); 

/* NOTE: 'correct' free list determined by either obj_size, or by canonical address */

/* Take from correct free list, and put @ mapping */
void get_mapping(size_t obj_size, mapping_t* mapping);

/* Put into correct free list -- return true if successful (i.e., correct freelist not full) */
bool put_mapping(void* vaddr, off_t canonical_addr);
