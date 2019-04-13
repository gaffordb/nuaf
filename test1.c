#include <stdio.h>
#include <stdlib.h>

int main(void) {
  printf("Making allocations...\n");
  void* p1 = malloc(512);
  void* p2 = malloc(1024);
  void* p3 = malloc(64);
  void* p4 = malloc(5);

  
  free(p1);
  free(p2);
  free(p3);

  printf("Dereferencing okay pointer...\n");
  int x = *(int*)p4;
  printf("Dereferencing bad pointer...\n");
  x = *(int*)p3;
  printf("Should have segfaulted...\n");
}
