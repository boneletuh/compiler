#ifndef ERRORS_H_
#define ERRORS_H_

#include <stdarg.h>


// predefine symbols
void error(const char * string);
void errorf(const char * string, ...);
void implementation_error(const char *);
void warning(const char *);


#include "tokenizer.h"


// general error function
void error(const char * string) {
  printf("Error: %s\n", string);
  exit(1);
}

// a printf() like error function, displays an error and exits the aplication
// format is the string to be printed, and has to be null terminated
// the format(s) must have the following syntax: %? . Being `?` the type of the format
// the type of format available are:
//  %t    prints a token
//  %s    prints a string
//  %c    prints a character as ASCII
//  %d    prints a signed integer in decimal
//  %%    prints a '%'
void errorf(const char * format, ...) {
  va_list args;
  // count the number of format specifiers inside the format string
  int count = 0;
  for (int i = 0; format[i] != '\0'; i++) {
    // if the string matches "%" at the current index increase the count
    if (format[i] == '%') {
      count++;
    }
  }
  va_start(args, count);
  // print the text interlaced with the format
  for (int i = 0; format[i] != '\0'; i++) {
    const char symbol = format[i];
    if (symbol == '%') {
      // (reading the next symbol is a correct because there sould always be the extra symbol: '\0')
      const char next_sym = format[i+1];
      if (next_sym == 't') {
        Token token = va_arg(args, Token);
        printf("%.*s", token.length, token.beginning);
      } else if (next_sym == 's') {
        char * str = va_arg(args, char *);
        printf("%s", str);
      } else if (next_sym == 'c') {
        char chr = (char)va_arg(args, int);
        putchar(chr);
      } else if (next_sym == 'd') {
        int number = va_arg(args, int);
        printf("%d", number);
      } else if (next_sym == '%') {
        putchar('%');
      } else { // unkown format specifier
        printf("\n[incorrect format in errorf() function]\n");
        exit(1);
      }
      // skip the next symbol and continue
      i++; continue;
    }
    putchar(symbol);
  }
  exit(1);
}

// error that should only appeard while developing the compiler
// the user of the language should not see this type of error
void implementation_error(const char * string) {
  printf("Implementation Error: %s\n", string);
  exit(2);
}


// general warning fuction
void warning(const char * string) {
  printf("Warning: %s\n", string);
}

#endif