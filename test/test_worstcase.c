#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

int main(int argc, char** argv) {
  
  if(argc != 4) {
    fprintf(stderr, "usage: %s <num_objs> <\"small\"/\"large\"> <should_free:(y/n)>\n", argv[0]);
    exit(1);
  }
  size_t num_allocs = atoi(argv[1]);
  bool is_small = *argv[2] == 's' ? true : false;
  bool should_free = *argv[3] == 'y' ? true : false;

  for(int i = 0; i < num_allocs; i++) {
    void* p = malloc(0x2000);
    if(should_free) {
      free(p);
    }
  }
  
  return 0;
}
