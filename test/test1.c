#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int main(void) {
  printf("Making allocations...\n");

  int p1_size = 512;
  int p2_size = 0x0900;
  int p3_size = 64;
  int p4_size = 5;
  int p5_size = 32;
  
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
  
  
  for(int i = 0; i < 1000; i++) {
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
    free(p5);

  }
  printf("Finished.\n");
  /*
  printf("Dereferencing okay pointer...\n");
  int x = *(int*)p5;
  printf("Dereferencing bad pointer...\n");
  x = *(int*)p3;
  printf("Should have segfaulted...\n");
  */
}
