#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("Making allocations...\n");
  for(int i = 0; i < 10000; i++) {
    void* p1 = malloc(512);
    void* p2 = malloc(0x4000);
    void* p3 = malloc(64);
    void* p4 = malloc(5);
    void* p5 = realloc(p4, 32);
  
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
