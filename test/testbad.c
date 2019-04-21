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

  void* pbad = malloc(512);
  free(pbad);

  printf("Trying bad dereference...\n");
  
  int x = *(int*)pbad;
  printf("Finished.%d\n", x); // using x so compiler is happy
}
