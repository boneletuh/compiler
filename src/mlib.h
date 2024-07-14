#ifndef MLIB_H_
#define MLIB_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// predefine symbols
void * smalloc(size_t);
void * srealloc(void *, size_t);
char * file_contents(const char *);
const char * get_file_extension(const char *);


#include "errors.h"


// its like malloc() but it checks that it could allocate memory
void * smalloc(size_t nbytes) {
  void * ptr = malloc(nbytes);
  if (ptr == NULL) {
    errorf("Execution Error: can not allocate memory\n");
  }
  return ptr;
}

// its like realloc() but it checks that it could allocate memory
void * srealloc(void * ptr, size_t nbytes) {
  void * new_ptr = realloc(ptr, nbytes);
  if (new_ptr == NULL) {
    errorf("Execution Error: can not reallocate memory\n");
  }
  return new_ptr;
}

static int file_size(FILE * file) {
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fseek(file, 0, SEEK_SET);
  return size;
}

// returns the bytes of a file and closes it
char * file_contents(const char * file_path) {
  // open the source code file
  FILE * fptr = fopen(file_path, "r");
  if (fptr == NULL) {
    errorf("File Error: Can not open the input code file: %s\n", file_path);
  }
  const int size = file_size(fptr);
  char * out = smalloc(size + 1);
  for (int i = 0; i < size; i++) {
    out[i] = getc(fptr);
  }
  out[size] = '\0';
  
  fclose(fptr); 
  return out;
}

FILE * create_file(const char * file_name) {
  FILE * fptr = fopen(file_name, "w");
  if (fptr == NULL) {
    errorf("File Error: Can not create the file: %s\n", file_name);
  }
  return fptr; 
}

// returns a ptr to the beginning of the extension in the string
// the extension start at the last dot in the file name, including the dot
const char * get_file_extension(const char * file_name) {
  const char * last_dot = NULL;
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