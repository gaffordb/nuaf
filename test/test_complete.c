#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

int get_random_size(bool is_small) {
  if(is_small) {
    return (rand() % 1024) + 1;
  } else {
    return rand() % (1 << 17);
  }
}

void do_test(void) {
  int p1_size = get_random_size(true);
  int p2_size = get_random_size(true);
  int p3_size = get_random_size(false);
  int p4_size = get_random_size(true);
  int p5_size = get_random_size(true);

  /*
  fprintf(stderr, "%d, %d, %d, %d, %d\n",
          p1_size, p2_size,
          p3_size, p4_size,
          p5_size);
  */
    
  char p1_val[p1_size];
  char p2_val[p2_size];
  char p3_val[p3_size];
  char p4_val[p4_size];
  char p5_val[p5_size];

  memset(p1_val, 1, p1_size);
  memset(p2_val, 2, p2_size);
  memset(p3_val, 3, p3_size);
  memset(p4_val, 4, p4_size);
  memset(p5_val, 5, p5_size);
  
  for(int i = 0; i < 50; i++) {
    void* p1 = malloc(p1_size);
    void* p2 = malloc(p2_size);
    void* p3 = malloc(p3_size);
    void* p4 = malloc(p4_size);
    void* p5 = malloc(p5_size);

    memset(p1, 1, p1_size);
    memset(p2, 2, p2_size);
    memset(p3, 3, p3_size);
    memset(p4, 4, p4_size);
    memset(p5, 5, p5_size);

    assert(!memcmp(p1, p1_val, p1_size));
    assert(!memcmp(p2, p2_val, p2_size));
    assert(!memcmp(p3, p3_val, p3_size));
    assert(!memcmp(p4, p4_val, p4_size));
    assert(!memcmp(p5, p5_val, p5_size));
    
    free(p1);
    free(p2);
    free(p3);
    free(p4);
    free(p5);
  }
}

int main(void) {
  srand(time(NULL));

  void* p = malloc(0x4000);
  
  printf("Making allocations...\n");

  for(int i = 0; i < 2500; i++) {
    do_test();
  }
  free(p);
  for(int i = 0; i < 2500; i++) {
    do_test();
  }
  
  printf("Finished allocations.\n");

  printf("Testing big object uaf... (Should segfault)\n");
  *(int*)p = 5;
  
}
