#include "comp.h"


int main(int argc, char ** argv) {
  if (argc != 3) {
    error("invalid number of cmd arguments, you must write:\n  compiler [input file path] [output file path]");
  }
  // start clock
  clock_t start = clock();

  // TODO: improve cmd args handling
  char * code = argv[1];
  char * out_file = argv[2];

  compile(code, out_file);

  // time it
  float time = ((float) (clock() - start)) / CLOCKS_PER_SEC;
  printf("#######\ntime: %f\n", time);

  printf("compilation ended\n");
  return 0;
}
