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
    Curly_bracket,
    Bracket,
    End_of_file
  } type;
} Token;

// TODO: add look up table for O(1) time
bool is_in_str(char symbol, const char * string) {
  for (int i = 0; string[i] != '\0'; i++) {
    if (string[i] == symbol) {
      return true;
    }
  }
  return false;
}


Token * append_token(Token * tokens, int tokens_count, const Token token) {
  tokens_count += 1;
  tokens = srealloc(tokens, sizeof(Token) * (tokens_count + 1));
  tokens[tokens_count -1] = token;
  return tokens;
}

// TODO: add linked list for O(1) time when appending tokens
Token * lexer(char * string) {
  typedef enum {
    searching_token,
    identifier,
    number,
    operation
  } State;
  State mode = searching_token;

  const char * var_sym = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char * numb_sym = "0123456789";
  const char * oper_sym = "+-*/=%^><";
  const char * separ_sym = " \r\t\n";

  int token_beginning = 0;
  int token_count = 0;
  Token * token_array = malloc(token_count * sizeof(Token));
  Token new_token;
  int i;
  for (i = 0; string[i] != '\0'; i++) {
    const char symbol = string[i];
    switch (mode) {
      case searching_token:
        token_beginning = i;
        if (is_in_str(symbol, var_sym)) {
          mode = identifier;
          i--;
        }

        else if (is_in_str(symbol, numb_sym)) {
          mode = number;
          i--;
        }

        else if (is_in_str(symbol, oper_sym)) {
          mode = operation;
          i--;
        }

        // deal here with sigle symbol tokens
        else if (symbol == ':') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Colon;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }

        else if (symbol == ';') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Semi_colon;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }

        else if (symbol == '{' || symbol == '}') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Curly_bracket;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }
        else if (symbol == '(' || symbol == ')') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Bracket;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }
        // throw error if the symbol is not allowed
        else if (!is_in_str(symbol, separ_sym)) {
          // FIX: at the end of the file it sometimes detects the -1 symbol probably because utf-8
          if (symbol != -1) {
            implementation_error("unkown type of symbol");
          }
        }
        
        break;
      
      case identifier:
        new_token.type = Identifier;
        if (!is_in_str(symbol, var_sym)) {
          new_token.beginning = string + token_beginning;
          new_token.length = (unsigned short) (i - token_beginning);
          new_token.type = Identifier;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
          i--;
        }

        break;
      
      case number:
        new_token.type = Number;
        if (!is_in_str(symbol, numb_sym)) {
          new_token.beginning = string + token_beginning;
          new_token.length = (unsigned short) (i - token_beginning);
          new_token.type = Number;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          
          token_beginning = i;
          mode = searching_token;
          i--;
        }

        break;
      
      case operation:
        new_token.type = Operation;
        if (!is_in_str(symbol, oper_sym)) {
          new_token.beginning = string + token_beginning;
          new_token.length = (unsigned short) (i - token_beginning);
          new_token.type = Operation;

          // add token to token_array
          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
          i--;
        }

        break;

      default:
        implementation_error("unkown tokenizer State");
    }
  }
  // if theres still a token left add it to the token array
  if (mode != searching_token) {
    new_token.beginning = string + token_beginning;
    new_token.length = (unsigned short) (i - token_beginning);

    token_array = append_token(token_array, token_count, new_token);
    token_count += 1;
  }
  // add EOF token to the end of token array
  new_token.beginning = NULL;
  new_token.length = 0;
  new_token.type = End_of_file;

  token_array = append_token(token_array, token_count, new_token);

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

bool compare_str_of_tokens(Token token1, Token token2) {
  if (token1.length != token2.length) {
    return false;
  }
  for (int i = 0; i < token1.length; i++) {
    if (token1.beginning[i] != token2.beginning[i]) {
      return false;
    }
  }
  return true;
}


#endif