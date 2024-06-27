#ifndef CHECKER_H_
#define CHECKER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"

// TODO: use hashmap instead of array
// array with the variables inside a scope
typedef struct Scope {
  int vars_count;
  Token * vars;
} Scope;

// array with all the scopes
typedef struct Vars_list {
  int scopes_count;
  Scope * scopes;
} Vars_list;

// checkes if the token is in the list of variables
bool is_var_in_var_list(Vars_list vars, Token variable) {
  for (int i = 0; i < vars.scopes_count; i++) {
    for (int j = 0; j < vars.scopes[i].vars_count; j++) {
      if (compare_str_of_tokens(variable, vars.scopes[i].vars[j])) {
        return true;
      }
    }
  }
  return false;
}

// checks if there is some undeclared var in the expresion, if so it throws an error
bool is_expresion_valid(Vars_list scopes, Node_Expresion expresion) {
  if (expresion.expresion_type == expresion_identifier_type) {
    if (!is_var_in_var_list(scopes, expresion.expresion_value.expresion_identifier_value)) {
      error("undeclared variable used");
    }
  }
  else if (expresion.expresion_type == expresion_binary_operation_type) {
    is_expresion_valid(scopes, expresion.expresion_value.expresion_binary_operation_value->left_side);
    is_expresion_valid(scopes, expresion.expresion_value.expresion_binary_operation_value->right_side);
  }
  return true;
}

// append a variable to the array of variables in the last scope
void append_var_to_var_list(const Token variable, Vars_list * scopes) {
  Scope * last_scope = &scopes->scopes[scopes->scopes_count -1];
  last_scope->vars_count++;
  last_scope->vars = srealloc(last_scope->vars, last_scope->vars_count * sizeof(Token));
  last_scope->vars[last_scope->vars_count -1] = variable;
}

// create a new empty scope and append it to the end of array of scopes
void create_scope(Vars_list * scopes) {
  scopes->scopes_count++;
  scopes->scopes = srealloc(scopes->scopes, scopes->scopes_count * sizeof(*scopes->scopes));
  scopes->scopes[scopes->scopes_count -1].vars_count = 0;
  scopes->scopes[scopes->scopes_count -1].vars = smalloc(scopes->scopes[scopes->scopes_count -1].vars_count * sizeof(Token));
}

// create a copy of the scopes and its variables
Vars_list copy_vars_list(Vars_list scopes) {
  Vars_list result;
  result.scopes_count = scopes.scopes_count;
  result.scopes = smalloc(scopes.scopes_count * sizeof(*scopes.scopes));
  for (int i = 0; i < scopes.scopes_count; i++) {
    result.scopes[i].vars_count = scopes.scopes[i].vars_count;
    result.scopes[i].vars = smalloc(result.scopes[i].vars_count * sizeof(*result.scopes[i].vars));
    for (int j = 0; j < result.scopes[i].vars_count; j++) {
      result.scopes[i].vars[j] = scopes.scopes[i].vars[j];
    }
    //memcpy(result.scopes[i].vars, scopes.scopes[i].vars, result.scopes[i].vars_count * sizeof(*result.scopes[i].vars));
  }
  return result;
}

// free the memory of the scopes
void free_vars_list(Vars_list variables) {
  for (int i = 0; i < variables.scopes_count; i++ ) {
    free(variables.scopes[i].vars);
  }
  free(variables.scopes);
}

// check if a statement is valid, if it is not, report it and halt
void check_statement(Vars_list * variables, Node_Statement stmt) {
  switch (stmt.statement_type) {
    case var_declaration_type: {
      // check that when declaring a var there isnt another var with the same name
      Token variable = stmt.statement_value.var_declaration.var_name;
      if (is_var_in_var_list(*variables, variable)) {
        error("variable already declared");
      }
      else {
        append_var_to_var_list(variable, variables);
      }
      if (!compare_token_to_string(stmt.statement_value.var_declaration.type.type, "u64")) {
        error("invalid type in variable declaration");
      }
      // check that the expresion in the statement is valid
      Node_Expresion expresion = stmt.statement_value.var_declaration.value;
      is_expresion_valid(*variables, expresion);
      break;
    }
    case exit_node_type: {
      Node_Expresion expresion = stmt.statement_value.exit_node.exit_code;
      is_expresion_valid(*variables, expresion);
      break;
    }
    case print_type: {
      Node_Expresion expresion = stmt.statement_value.print.chr;
      is_expresion_valid(*variables, expresion);
      break;
    }
    case var_assignment_type: {
      // check that when assigning to a var there is another var with the same name
      Token variable = stmt.statement_value.var_assignment.var_name;
      if (!is_var_in_var_list(*variables, variable)) {
        error("variable has not been declared before");
      }
      // check that the expresion is valid
      Node_Expresion expresion = stmt.statement_value.var_assignment.value;
      is_expresion_valid(*variables, expresion);
      break;
    }
    case scope_type: {
      Vars_list scope_vars = copy_vars_list(*variables);
      for (int i = 0; i < stmt.statement_value.scope.statements_count; i++) {
        check_statement(&scope_vars, stmt.statement_value.scope.statements_node[i]);
      }
      free_vars_list(scope_vars);
      break;
    }
    case if_type: {
      Node_Expresion condition = stmt.statement_value.if_node.condition;
      is_expresion_valid(*variables, condition);
      Vars_list scope_vars = copy_vars_list(*variables);
      for (int i = 0; i < stmt.statement_value.if_node.scope.statements_count; i++) {
        check_statement(&scope_vars, stmt.statement_value.if_node.scope.statements_node[i]);
      }
      if (stmt.statement_value.if_node.has_else_block) {
        for (int i = 0; i < stmt.statement_value.if_node.else_block.statements_count; i++) {
          check_statement(&scope_vars, stmt.statement_value.if_node.else_block.statements_node[i]);
        }
      }
      free_vars_list(scope_vars);
      break;
    }
    case while_type: {
      Node_Expresion condition = stmt.statement_value.while_node.condition;
      is_expresion_valid(*variables, condition);
      Vars_list scope_vars = copy_vars_list(*variables);
      for (int i = 0; i < stmt.statement_value.while_node.scope.statements_count; i++) {
        check_statement(&scope_vars, stmt.statement_value.while_node.scope.statements_node[i]);
      }
      free_vars_list(scope_vars);
      break;
    }
    default:
      implementation_error("generating this statement is not implemented");
  }
}

// checks if the program follows the grammar rules and the language specifications
bool is_valid_program(Node_Program program) {
  Vars_list scopes;
  scopes.scopes_count = 0;
  scopes.scopes = malloc(scopes.scopes_count * sizeof(Scope));
  create_scope(&scopes); // create the first global scope
  // check each statement correctness
  for (int i = 0; i < program.statements_count; i++) {
    Node_Statement statement = program.statements_node[i];
    check_statement(&scopes, statement);
  }
  free_vars_list(scopes);
  return true;
}

#endif