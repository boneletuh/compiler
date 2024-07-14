#ifndef TOKENIZER_H_
#define TOKENIZER_H_

#include "errors.h"
#include "mlib.h"

#define NULL_TOKEN (Token) { .beginning=NULL, .length=0, .type=End_of_file }


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
  // extra info for better error messages
  int line_number;
  short column_number;
} Token;


#ifdef DEBUG
void D_print_tokens(const Token * tokens, const int size) {
  for (int i = 0; i < size; i++) {
    Token token = tokens[i];
    printf("%.*s\n", token.length, token.beginning);
  }
}
#endif


bool is_in_str(char symbol, const char * string) {
  for (int i = 0; string[i] != '\0'; i++) {
    if (string[i] == symbol) {
      return true;
    }
  }
  return false;
}

// appends a token to the end of the array of tokens
static Token * append_token(Token * tokens, int tokens_count, const Token token) {
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
  const char * oper_sym = "+-*/%^><&";
  const char * separ_sym = " \r\t\n";

  int token_beginning = 0;
  int token_count = 0;
  Token * token_array = malloc(token_count * sizeof(Token));
  Token new_token = NULL_TOKEN;

  int line_number = 1;
  int column_number = 1;

  int i;
  for (i = 0; string[i] != '\0'; i++) {
    column_number++;
    const char symbol = string[i];
    switch (mode) {
      case searching_token:
        token_beginning = i;
        if (is_in_str(symbol, var_sym)) {
          mode = identifier;
          i--;
          column_number--;
        }

        else if (is_in_str(symbol, numb_sym)) {
          mode = number;
          i--;
          column_number--;
        }

        else if (is_in_str(symbol, oper_sym)) {
          mode = operation;
          i--;
          column_number--;
        }

        else if (symbol == '=') {
          // check for equality operator
          if (string[i + 1] == '=') {
            new_token.beginning = string + token_beginning;
            new_token.length = 2;
            new_token.type = Operation;
            new_token.line_number = line_number;
            new_token.column_number = column_number;

            token_array = append_token(token_array, token_count, new_token);
            token_count++;

            // add 1 because the token is 1 character longer
            i += 1;
            token_beginning = i;
            mode = searching_token;            
          } // otherwise it is an assignment
          else {
            new_token.beginning = string + token_beginning;
            new_token.length = 1;
            new_token.type = Operation;
            new_token.line_number = line_number;
            new_token.column_number = column_number;

            token_array = append_token(token_array, token_count, new_token);
            token_count++;

            token_beginning = i;
            mode = searching_token;
          }
        }

        // deal here with sigle symbol tokens
        else if (symbol == ':') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Colon;
          new_token.line_number = line_number;
          new_token.column_number = column_number;

          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }

        else if (symbol == ';') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Semi_colon;
          new_token.line_number = line_number;
          new_token.column_number = column_number;

          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }

        else if (symbol == '{' || symbol == '}') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Curly_bracket;
          new_token.line_number = line_number;
          new_token.column_number = column_number;

          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }
        else if (symbol == '(' || symbol == ')') {
          new_token.beginning = string + token_beginning;
          new_token.length = 1;
          new_token.type = Bracket;
          new_token.line_number = line_number;
          new_token.column_number = column_number;

          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
        }
        else if (symbol == '\n') {
          line_number += 1;
          column_number = 1;
        }
        // throw error if the symbol is not allowed
        else if (!is_in_str(symbol, separ_sym)) {
          errorf("Line:%d, column:%d.  Error: unkown type of symbol (%c)\n", line_number, column_number, symbol);
        }
        
        break;
      
      case identifier:
        new_token.type = Identifier;
        if (!is_in_str(symbol, var_sym) && !is_in_str(symbol, numb_sym)) {
          new_token.beginning = string + token_beginning;
          new_token.length = (unsigned short) (i - token_beginning);
          new_token.type = Identifier;
          new_token.line_number = line_number;
          new_token.column_number = column_number;

          token_array = append_token(token_array, token_count, new_token);
          token_count++;

          token_beginning = i;
          mode = searching_token;
          i--;
          column_number--;
        }

        break;
      
      case number:
        new_token.type = Number;
        if (!is_in_str(symbol, numb_sym)) {
          new_token.beginning = string + token_beginning;
          new_token.length = (unsigned short) (i - token_beginning);
          new_token.type = Number;
          new_token.line_number = line_number;
          new_token.column_number = column_number;

          token_array = append_token(token_array, token_count, new_token);
          token_count++;
          
          token_beginning = i;
          mode = searching_token;
          i--;
          column_number--;
        }

        break;
      
      case operation:
        // the operation tokens (that have not been handled already) are 1 character large
        new_token.beginning = string + token_beginning;
        new_token.length = 1;
        new_token.type = Operation;
        new_token.line_number = line_number;
        new_token.column_number = column_number;

        token_array = append_token(token_array, token_count, new_token);
        token_count++;

        token_beginning = i;
        mode = searching_token;

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
  // append null token to mark the end of the array
  token_array = append_token(token_array, token_count, NULL_TOKEN);

  return token_array;
}


bool compare_token_to_string(const Token token, const char * string) {
  int i;
  for (i = 0; i < token.length && string[i] != '\0'; i++) {
    if (token.beginning[i] != string[i]) {
      return false;
    }
  }

  return i == token.length && string[i] == '\0';
}

bool compare_str_of_tokens(const Token token1, const Token token2) {
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