#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include "errors.h"
#include "mlib.h"

typedef struct Token {
  char * beginning;
  unsigned short length;
  enum {
    Identifier,
    Number,
    Operation,
    Colon,
    Semi_colon,
    End_of_file
  } type;
} Token;

void add_token_to_file(FILE * file_ptr, Token token) {
  fprintf(file_ptr, "%.*s", token.length, token.beginning);
}

bool is_in_str(char symbol, const char * string) {
  // TODO: add look up table for O(1) time
  for (int i = 0; string[i] != '\0'; i++) {
    if (string[i] == symbol) {
      return true;
    }
  }
  return false;
}

Token * lexer(char * string) {
  typedef enum {
    searching_token,
    identifier,
    number,
    operation,
    add_token
  } State;
  State mode = searching_token;

  const char * var_sym = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char * numb_sym = "0123456789";
  const char * oper_sym = "+-*/=%^";
  const char * separ_sym = " \r\t\n";

  int token_beginning = 0;
  int token_count = 0;
  Token * token_array = malloc(token_count * sizeof(Token));
  Token new_token;
  int i;
  for (i = 0; string[i] != '\0'; i++) {
    const char simbol = string[i];
    switch (mode) {
      case searching_token:
        token_beginning = i;
        if (is_in_str(simbol, var_sym)) {
          mode = identifier;
          i -= 1;
        }

        else if (is_in_str(simbol, numb_sym)) {
          mode = number;
          i -= 1;
        }

        else if (is_in_str(simbol, oper_sym)) {
          mode = operation;
          i -= 1;
        }

        // deal here with sigle symbol tokens
        else if (simbol == ':') {
          mode = add_token;
          new_token.type = Colon;
        }

        else if (simbol == ';') {
          mode = add_token;
          new_token.type = Semi_colon;
        }
        // throw error if the symbol is not allowed
        else if (!is_in_str(simbol, separ_sym)) {
          // FIX: at the end of the file it sometimes detects the -1 symbol probably because utf-8
          if (simbol != -1) {
            implementation_error("unkown type of symbol");
          }
        }
        
        break;
      
      case identifier:
        new_token.type = Identifier;
        if (!is_in_str(simbol, var_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case number:
        new_token.type = Number;
        if (!is_in_str(simbol, numb_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case operation:
        new_token.type = Operation;
        // the operation can be longer than 1 symbol
        if (!is_in_str(simbol, oper_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case add_token:
        new_token.beginning = string + token_beginning;
        new_token.length = (unsigned short) (i - token_beginning);

        // add token to token_array
        Token * new_token_array = realloc(token_array, sizeof(Token) * (token_count + 1));
        if (new_token_array == NULL) { 
          allocation_error("can not allocate memory for new token");
        }

        token_array = new_token_array;

        token_array[token_count] = new_token;

        //this is unnecessary just to clean the new_token
        new_token.beginning = NULL;
        new_token.length = 0;
        new_token.type = End_of_file;
        
        // update token beginning
        token_beginning = i;

        mode = searching_token;
        i -= 1;
        token_count += 1;
        break;

      default:
        implementation_error("unkown tokenizer State");
    }
  }
  // if theres still a token left add it to the token array
  if (mode != searching_token) {
    new_token.beginning = string + token_beginning;
    new_token.length = (unsigned short) (i - token_beginning);

    // add token to token_array
    Token * new_token_array = realloc(token_array, sizeof(Token) * (token_count + 1));
    if (new_token_array == NULL) { 
      allocation_error("can not allocate memory for last token");
    }

    token_array = new_token_array;

    token_array[token_count].beginning = new_token.beginning;
    token_array[token_count].length = new_token.length;
    token_array[token_count].type = new_token.type;
    
    token_count += 1;
  }
  // add EOF token to the end of token array
  Token * new_token_array = realloc(token_array, sizeof(Token) * (token_count + 1));
  if (new_token_array == NULL) { 
    allocation_error("can not allocate memory for EOF token");
  }

  token_array = new_token_array;

  token_array[token_count].beginning = NULL;
  token_array[token_count].length = 0;
  token_array[token_count].type = End_of_file;
  return token_array;
}


bool compare_token_to_string(Token token, const char * string) {
  int i;
  for (i = 0; i < token.length && string[i] != '\0'; i++) {
    if (token.beginning[i] != string[i]) {
      return false;
    }
  }

  if (i == token.length && string[i] == '\0') {
    return true;
  }
  else {
    return false;
  }
}

#endif