#ifndef MLIB_H_
#define MLIB_H_

#include <stdlib.h>
#include <stdio.h>

#include "errors.h"

int file_size(FILE * file) {
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fseek(file, 0, SEEK_SET);
  return size;
}

char * file_contents(FILE * fptr) {
  if (fptr == NULL) {
    file_error("can not open the input code file");
  }

  int size = file_size(fptr);
  char * out = (char *) malloc(size + 1);
  if (out == NULL) {
    allocation_error("can not allocate memory for the program source code");
  }

  for (int i = 0; i < size; i++) {
    out[i] = getc(fptr);
  }
  out[size] = '\0';
  
  fclose(fptr); 
  return out;
}

typedef enum bool {
  false,
  true
} bool;

#endif
