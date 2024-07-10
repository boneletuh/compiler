#ifndef CHECKER_H_
#define CHECKER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"

// TODO: use hashmap instead of array
// array with the variables inside a scope
typedef struct Symbol {
  Token value;
  Node_Type type;
} Symbol;

typedef struct Scope {
  int vars_count;
  Symbol * vars;
} Scope;

// array with all the scopes
typedef struct Vars_list {
  int scopes_count;
  Scope * scopes;
} Vars_list;

// returns if the 2 types are equal
bool compare_2_types(const Node_Type type1, const Node_Type type2) {
  if (type1.type_type != type2.type_type) {
    return false;
  }
  // does not matter if it is type1.type_type or type2.type_type
  if (type1.type_type == type_primitive_type) {
    Token token_type1 = type1.type_value.type_primitive_value;
    Token token_type2 = type2.type_value.type_primitive_value;
    return compare_str_of_tokens(token_type1, token_type2);
  }
  else if (type1.type_type == type_ptr_type) {
    Node_Type tmp_type1 = *type1.type_value.type_ptr_value;
    Node_Type tmp_type2 = *type2.type_value.type_ptr_value;
    return compare_2_types(tmp_type1, tmp_type2);
  }
  else {
    implementation_error("checking this type of type not implemented");
  }

  return true;
}

// returns the symbol associated with the token
// if it could not find it, it throws an error
Symbol get_symbol_from_token(const Vars_list vars, const Token token) {
  for (int i = 0; i < vars.scopes_count; i++) {
    Scope scope = vars.scopes[i];
    for (int j = 0; j < scope.vars_count; j++) {
      Symbol symbol = scope.vars[j];
      if (compare_str_of_tokens(token, symbol.value)) {
        return symbol;
      }
    }
  }
  implementation_error("could not get the associated symbol from token in checker");
  // unrecheable
  return (Symbol) {};
}

// returns the type of a given expresion
Node_Type get_type_of_expresion(const Vars_list vars, const Node_Expresion expresion) {
  if (expresion.expresion_type == expresion_identifier_type) {
    Token identifier = expresion.expresion_value.expresion_identifier_value;
    Symbol symbol = get_symbol_from_token(vars, identifier);
    return symbol.type;
  }
  if (expresion.expresion_type == expresion_unary_operation_type) {
    Node_Unary_Operation uni_operation = *expresion.expresion_value.expresion_unary_operation_value;
    Node_Expresion uni_expresion = uni_operation.expresion;
    Node_Type type;
    // if the operation is the address operator `&`, the returned type is a pointer to the type of the expression
    if (uni_operation.operation_type == unary_operation_addr_type) {
      type.token = NULL_TOKEN;
      type.type_type = type_ptr_type;
      type.type_value.type_ptr_value = smalloc(sizeof(*type.type_value.type_ptr_value)); // FIX: this leaks memory, too bad
      *type.type_value.type_ptr_value = get_type_of_expresion(vars, uni_expresion);
    }
    // if the operation is the dereference operator `*`, the returned type is type the pointer holds
    else if (uni_operation.operation_type == unary_operation_deref_type) {
      type = *get_type_of_expresion(vars, uni_expresion).type_value.type_ptr_value;
    }
    else {
      type = get_type_of_expresion(vars, uni_expresion);
    }
    return type;
  }
  if (expresion.expresion_type == expresion_binary_operation_type) {
    Node_Binary_Operation bin_operation = *expresion.expresion_value.expresion_binary_operation_value;
    Node_Expresion lhs_expr = bin_operation.left_side;
    Node_Expresion rhs_expr = bin_operation.right_side;
    Node_Type lhs_type = get_type_of_expresion(vars, lhs_expr);
    Node_Type rhs_type = get_type_of_expresion(vars, rhs_expr);
    // if any of the operands is a pointer throw an error
    if (lhs_type.type_type == type_ptr_type || rhs_type.type_type == type_ptr_type) {
      error("can not operate with a pointer");
    }
    // does not matter if its `lhs_type` or `rhs_type` 
    return lhs_type;
  }
  if (expresion.expresion_type == expresion_number_type) {
    // FIX: can not get the type of an integer literal so assume it is `u64`
    Node_Type type;
    type.token.beginning = smalloc(4); // FIX: this also leaks memory
    strcpy(type.token.beginning, "u64");
    type.token.length = 3;
    type.type_type = type_primitive_type;
    type.type_value.type_primitive_value = type.token;
    return type;
  }
  implementation_error("unkown type of expresion while trying to get its type");
  return (Node_Type) {};
}

// checkes if the token is in the list of variables
bool is_var_in_var_list(const Vars_list vars, const Token variable) {
  for (int i = 0; i < vars.scopes_count; i++) {
    Scope scope = vars.scopes[i];
    for (int j = 0; j < scope.vars_count; j++) {
      Symbol symbol = scope.vars[j];
      if (compare_str_of_tokens(variable, symbol.value)) {
        return true;
      }
    }
  }
  return false;
}

// checks if there is some undeclared var in the expresion, if so it throws an error
bool is_expresion_valid(const Vars_list scopes, const Node_Expresion expresion) {
  if (expresion.expresion_type == expresion_identifier_type) {
    if (!is_var_in_var_list(scopes, expresion.expresion_value.expresion_identifier_value)) {
      error("undeclared variable used");
    }
  }
  else if (expresion.expresion_type == expresion_binary_operation_type) {
    is_expresion_valid(scopes, expresion.expresion_value.expresion_binary_operation_value->left_side);
    is_expresion_valid(scopes, expresion.expresion_value.expresion_binary_operation_value->right_side);
  }
  else if (expresion.expresion_type == expresion_unary_operation_type) {
    Node_Unary_Operation uni_operation = *expresion.expresion_value.expresion_unary_operation_value;
    if (uni_operation.operation_type == unary_operation_addr_type) {
      // can only get the address of a variable
      if (uni_operation.expresion.expresion_type != expresion_identifier_type) {
        error("can only take the address of a variable");
      }
    }
    else if (uni_operation.operation_type == unary_operation_deref_type) {
      // can only dereference a pointer
      if (get_type_of_expresion(scopes, uni_operation.expresion).type_type != type_ptr_type) {
        error("can only dereference a pointer");
      }
    }
    else {
      implementation_error("unkown type of unary operation while checking");
    }
    is_expresion_valid(scopes, uni_operation.expresion);
  }
  return true;
}

// append a variable to the array of variables in the last scope
void append_var_to_var_list(const Symbol variable, Vars_list * scopes) {
  Scope * last_scope = &scopes->scopes[scopes->scopes_count -1];
  last_scope->vars_count++;
  last_scope->vars = srealloc(last_scope->vars, last_scope->vars_count * sizeof(*last_scope->vars));
  last_scope->vars[last_scope->vars_count -1] = variable;
}

// create a new empty scope and append it to the end of array of scopes
void create_scope(Vars_list * scopes) {
  scopes->scopes_count++;
  scopes->scopes = srealloc(scopes->scopes, scopes->scopes_count * sizeof(*scopes->scopes));
  scopes->scopes[scopes->scopes_count -1].vars_count = 0;
  scopes->scopes[scopes->scopes_count -1].vars = smalloc(scopes->scopes[scopes->scopes_count -1].vars_count * sizeof(*scopes->scopes[scopes->scopes_count -1].vars));
}

// create a copy of the scopes and its variables
Vars_list copy_vars_list(Vars_list scopes) {
  Vars_list result;
  result.scopes_count = scopes.scopes_count;
  result.scopes = smalloc(scopes.scopes_count * sizeof(*scopes.scopes));
  for (int i = 0; i < scopes.scopes_count; i++) {
    result.scopes[i].vars_count = scopes.scopes[i].vars_count;
    result.scopes[i].vars = smalloc(result.scopes[i].vars_count * sizeof(*result.scopes[i].vars));
    // TODO; use memcpy() here
    for (int j = 0; j < result.scopes[i].vars_count; j++) {
      result.scopes[i].vars[j] = scopes.scopes[i].vars[j];
    }
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
void check_statement(Vars_list * variables, const Node_Statement stmt) {
  switch (stmt.statement_type) {
    case var_declaration_type: {
      // check that the expresion in the statement is valid
      Node_Expresion expresion = stmt.statement_value.var_declaration.value;
      is_expresion_valid(*variables, expresion);

      // check that when declaring a var there isnt another var with the same name
      Symbol variable = {
        .value=stmt.statement_value.var_declaration.var_name,
        .type=stmt.statement_value.var_declaration.type
      };
      if (is_var_in_var_list(*variables, variable.value)) {
        error("variable already declared");
      }
      else {
        append_var_to_var_list(variable, variables);
      }

      // check that the types of the declaration are valid with the ones of the expresion
      if (!compare_2_types(variable.type, get_type_of_expresion(*variables, expresion))) {
        error("invalid type in variable declaration");
      }
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
      // check that the types of the variable and the expression match
      Node_Type var_type = get_symbol_from_token(*variables, variable).type;
      Node_Type expr_type = get_type_of_expresion(*variables, expresion);
      if (!compare_2_types(var_type, expr_type)) {
        error("the type of the expression and the variable does not match");
      }
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