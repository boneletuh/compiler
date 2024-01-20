#ifndef CHECKER_H_
#define CHECKER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"

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
        var_list_size++;
        var_list = srealloc(var_list, var_list_size * sizeof(Token));
        var_list[var_list_size -1] = variable;
      }
      // check that the expresion in the statement is valid
      Node_Expresion expresion = statement.statement_value.var_declaration.value;
      is_expresion_valid(var_list, var_list_size, expresion);
    }
    else if (statement.statement_type == exit_node_type) {
      Node_Expresion expresion = statement.statement_value.exit_node.exit_code;
      is_expresion_valid(var_list, var_list_size, expresion);
    }
    else if (statement.statement_type == var_assignment_type) {
      // check that when assignmening to a var there is another var with the same name
      Token variable = statement.statement_value.var_assignment.var_name;
      if (!is_var_in_var_list(var_list, var_list_size, variable)) {
        error("variable has not been declared before");
      }
      // check that the expresion is good
      Node_Expresion expresion = statement.statement_value.var_assignment.value;
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