#ifndef CHECKER_H_
#define CHECKER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"

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

// checkes if the token is int list of tokens
bool is_var_in_var_list(Token * var_list, int var_list_size, Token variable) {
  for (int i = 0; i < var_list_size; i++) {
    if (compare_str_of_tokens(variable, var_list[i])) {
      return true;
    }
  }
  return false;
}

// checks if there is some undeclared var in the expresion, if so it throws an error
bool is_expresion_valid(Token * var_list, int var_list_size, Node_Expresion expresion) {
  if (expresion.expresion_type == expresion_identifier_type) {
    if (!is_var_in_var_list(var_list, var_list_size, expresion.expresion_value.expresion_identifier_value)) {
      error("undeclared variable used");
    }
  }
  else if (expresion.expresion_type == expresion_binary_operation_type) {
    is_expresion_valid(var_list, var_list_size, expresion.expresion_value.expresion_binary_operation_value->left_side);
    is_expresion_valid(var_list, var_list_size, expresion.expresion_value.expresion_binary_operation_value->right_side);
  }
  return true;
}


// checks if the program follows the grammar rules and the language specifications
bool is_valid_program(Node_Program program) {
  // TODO: use hashmap instead of array
  int var_list_size = 0;
  Token * var_list = malloc(var_list_size * sizeof(Token));
  for (int i = 0; i < program.statements_count; i++) {
    Node_Statement statement = program.statements_node[i];
    if (statement.statement_type == var_declaration_type) {
      // check that when declaring a var there isnt another var with the same name
      Token variable = statement.statement_value.var_declaration.var_name;
      if (is_var_in_var_list(var_list, var_list_size, variable)) {
        error("variable already declared");
      }
      else {
        var_list_size += 1;
        var_list = realloc(var_list, var_list_size * sizeof(Token));
        if (var_list == NULL) {
          allocation_error("could not allocate memory for variable tracker list");
        }
        var_list[var_list_size -1] = variable;
      }
      // check that when using a var it has been declared before
      Node_Expresion expresion = statement.statement_value.var_declaration.value;
      is_expresion_valid(var_list, var_list_size, expresion);
    }
    else if (statement.statement_type == exit_node_type) {
      Node_Expresion expresion = statement.statement_value.exit_node.exit_code;
      is_expresion_valid(var_list, var_list_size, expresion);
    }
    else {
      implementation_error("generating this statement is not implemented");
    }
  }
  free(var_list);
  return true;
}

#endif