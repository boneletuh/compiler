#ifndef MLIB_H_
#define MLIB_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "errors.h"

// its like malloc() but it checks that it could allocate memory
void * smalloc(size_t nbytes) {
  void * ptr = malloc(nbytes);
  if (ptr == NULL) {
    allocation_error("can not allocate memory");
  }
  return ptr;
}

// its like realloc() but it checks that it could allocate memory
void * srealloc(void * ptr, size_t nbytes) {
  void * new_ptr = realloc(ptr, nbytes);
  if (new_ptr == NULL) {
    allocation_error("can not allocate memory");
  }
  return new_ptr;
}

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
  char * out = smalloc(size + 1);
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

// returns a ptr to the beginning of the extension in the string
// the extension start at the last dot in the file name, including the dot
char * get_file_extension(char * file_name) {
  char * last_dot = NULL;
  int i;
  for (i = 0; file_name[i] != '\0'; i++) {
    // find the last dot
    if (file_name[i] == '.') {
      last_dot = file_name + i;
    }
  }
  // didnt find any dots in the name
  if (last_dot == NULL) {
    // return ptr to the '\0' of the file name
    last_dot = file_name + i;
  }
  return last_dot;
}

#endif
