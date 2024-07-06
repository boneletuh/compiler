#ifndef ERRORS_H_
#define ERRORS_H_

#include "mlib.h"

// TODO: improve error handling a bit more

// general error function
void error(char * string) {
  printf("Error: %s\n", string);
  exit(1);
}

// error that should only appeard while developing the compiler
// the user of the language sould not see this type of error
void implementation_error(const char * string) {
  printf("Implementation Error: %s\n", string);
  exit(2);
}

// for errors that happend while dealing with files
void file_error(const char * string) {
  printf("File Error: %s\n", string);
  exit(3);
}

// for errors that happend when calling malloc() or realloc() or something like that
void allocation_error(const char * string) {
  printf("Memory Allocation Error: %s\n", string);
  exit(4);
}

// general warning fuction
void warning(const char * string) {
  printf("Warning: %s\n", string);
}

#endif