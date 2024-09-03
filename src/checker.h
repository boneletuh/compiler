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

typedef struct Symbols_scope {
  int vars_count;
  Symbol * vars;
} Symbols_scope;

// array with all the scopes
typedef struct Symbol_table {
  int scopes_count;
  Symbols_scope * scopes;
} Symbol_table;

// returns if the 2 types are equal
static bool compare_2_types(const Node_Type type1, const Node_Type type2) {
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
  else if (type1.type_type == type_array_type) {
    Node_Array_type tmp_type1 = *type1.type_value.type_array_value;
    Node_Array_type tmp_type2 = *type2.type_value.type_array_value;
    return compare_2_types(*tmp_type1.primitive_type, *tmp_type2.primitive_type) && compare_str_of_tokens(tmp_type1.elements_count, tmp_type2.elements_count);
  }
  else {
    implementation_error("checking this type of type not implemented");
  }

  return true;
}

// returns the symbol associated with the token
// if it could not find it, it throws an error
static Symbol get_symbol_from_token(const Symbol_table vars, const Token token) {
  for (int i = 0; i < vars.scopes_count; i++) {
    Symbols_scope scope = vars.scopes[i];
    for (int j = 0; j < scope.vars_count; j++) {
      Symbol symbol = scope.vars[j];
      if (compare_str_of_tokens(token, symbol.value)) {
        return symbol;
      }
    }
  }
  implementation_error("could not get the associated symbol from token in checker");
  // unreachable
  return (Symbol) {};
}

// returns the type of a given expresion
Node_Type get_type_of_expresion(const Symbol_table vars, const Node_Expresion expresion) {
  switch (expresion.expresion_type) {
    case expresion_number_type: {
      // FIX: can not get the type of an integer literal so assume it is `u64`
      Node_Type type;
      type.token.beginning = smalloc(4); // FIX: this leaks memory
      strcpy(type.token.beginning, "u64");
      type.token.length = 3;
      type.type_type = type_primitive_type;
      type.type_value.type_primitive_value = type.token;
      return type;
      break;
    }
    case expresion_identifier_type: {
      Token identifier = expresion.expresion_value.expresion_identifier_value;
      Symbol symbol = get_symbol_from_token(vars, identifier);
      return symbol.type;
      break;
    }
    case expresion_unary_operation_type: {
      Node_Unary_Operation uni_operation = *expresion.expresion_value.expresion_unary_operation_value;
      Node_Expresion uni_expresion = uni_operation.expresion;
      Node_Type type;
      // if the operation is the address operator `&`, the returned type is a pointer to the type of the expression
      if (uni_operation.operation_type == unary_operation_addr_type) {
        type.token = NULL_TOKEN;
        type.type_type = type_ptr_type;
        type.type_value.type_ptr_value = smalloc(sizeof(*type.type_value.type_ptr_value)); // FIX: this leaks memory
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
      break;
    }
    case expresion_binary_operation_type: {
      Node_Binary_Operation bin_operation = *expresion.expresion_value.expresion_binary_operation_value;
      Node_Expresion lhs_expr = bin_operation.left_side;
      Node_Expresion rhs_expr = bin_operation.right_side;
      Node_Type lhs_type = get_type_of_expresion(vars, lhs_expr);
      Node_Type rhs_type = get_type_of_expresion(vars, rhs_expr);
      // if any of the operands is a pointer throw an error
      if (lhs_type.type_type == type_ptr_type || rhs_type.type_type == type_ptr_type) {
        error("can not operate with a pointer");
      }
      // if the operation is an array access
      if (bin_operation.operation_type == binary_operation_access_type) {
        // return the type the array contains
        return *lhs_type.type_value.type_array_value->primitive_type;
      }
      // does not matter if its `lhs_type` or `rhs_type` 
      return lhs_type;
      break;
    }
    case expresion_array_type: {
      Node_Type type;
      type.token = NULL_TOKEN;
      type.type_type = type_array_type;
      type.type_value.type_array_value = smalloc(sizeof(*type.type_value.type_array_value));
      type.type_value.type_array_value->primitive_type = smalloc(sizeof(*type.type_value.type_array_value->primitive_type));
      *type.type_value.type_array_value->primitive_type = get_type_of_expresion(vars, expresion.expresion_value.expresion_array_value->elements[0]);
      // FIX: convert the number to token in a more reasonable way
      type.type_value.type_array_value->elements_count.beginning = smalloc(2);
      type.type_value.type_array_value->elements_count.length = 2;
      type.type_value.type_array_value->elements_count.beginning[0] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 10;
      type.type_value.type_array_value->elements_count.beginning[1] = '0' + expresion.expresion_value.expresion_array_value->elements_count % 10;
      type.type_value.type_array_value->elements_count.type = Number;
      return type;
      break;
    }
  }
  implementation_error("unkown type of expresion while trying to get its type");
  return (Node_Type) {};
}

// checkes if the token is in the list of variables
static bool is_var_in_var_list(const Symbol_table vars, const Token variable) {
  for (int i = 0; i < vars.scopes_count; i++) {
    Symbols_scope scope = vars.scopes[i];
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
static bool is_expresion_valid(const Symbol_table scopes, const Node_Expresion expresion) {
  if (expresion.expresion_type == expresion_number_type) {
    // does not need to check the number
  }
  else if (expresion.expresion_type == expresion_identifier_type) {
    Token variable = expresion.expresion_value.expresion_identifier_value;
    if (!is_var_in_var_list(scopes, variable)) {
      errorf("Line:%d, column:%d.  Error: undeclared variable used\n", variable.line_number, variable.column_number);
    }
  }
  else if (expresion.expresion_type == expresion_binary_operation_type) {
    Node_Expresion lhs_expr = expresion.expresion_value.expresion_binary_operation_value->left_side;
    Node_Expresion rhs_expr = expresion.expresion_value.expresion_binary_operation_value->right_side;
    Node_Binary_Operation bin_operation = *expresion.expresion_value.expresion_binary_operation_value;
    if (bin_operation.operation_type == binary_operation_access_type) {
      Node_Type lhs_type = get_type_of_expresion(scopes, lhs_expr);
      Node_Type rhs_type = get_type_of_expresion(scopes, rhs_expr);
      // the type for the left side has to be an array and for the right side an 'u64'
      // TODO: make this condition nicer
      if (!(lhs_type.type_type == type_array_type && rhs_type.type_type == type_primitive_type && compare_token_to_string(rhs_type.token, "u64"))) {
        error("can only access an value that has array type, and with an integer index");
      }
    }
    is_expresion_valid(scopes, lhs_expr);
    is_expresion_valid(scopes, rhs_expr);
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
  else if (expresion.expresion_type == expresion_array_type) {
    Node_Array array = *expresion.expresion_value.expresion_array_value;
    if (array.elements_count == 0) {
      error("can not have an empty array in expresion");
      return false;
    }

    // check that every expresion inside the array is valid
    for (int i = 0; i < array.elements_count; i++) {
      if (!is_expresion_valid(scopes, array.elements[i])) {
        return false;
      }
    }
    // also check that every expression inside has the same type
    const Node_Type expected_type = get_type_of_expresion(scopes, array.elements[0]); 
    for (int i = 0; i < array.elements_count; i++) {
      Node_Type sub_expresion_type = get_type_of_expresion(scopes, array.elements[i]);
      if (!compare_2_types(expected_type, sub_expresion_type)) {
        errorf("Line:%d, column:%d.  Error: the elements inside the array does not have the same type\n", sub_expresion_type.token.line_number, sub_expresion_type.token.column_number);
      }
    }
  }
  else {
    implementation_error("checking this type of expresion is not implemented");
  }
  return true;
}

// append a variable to the array of variables in the last scope
static void append_var_to_var_list(const Symbol variable, Symbol_table * scopes) {
  Symbols_scope * last_scope = &scopes->scopes[scopes->scopes_count -1];
  last_scope->vars_count++;
  last_scope->vars = srealloc(last_scope->vars, last_scope->vars_count * sizeof(*last_scope->vars));
  last_scope->vars[last_scope->vars_count -1] = variable;
}

// create a new empty scope and append it to the end of array of scopes
static void create_scope(Symbol_table * scopes) {
  scopes->scopes_count++;
  scopes->scopes = srealloc(scopes->scopes, scopes->scopes_count * sizeof(*scopes->scopes));
  scopes->scopes[scopes->scopes_count -1].vars_count = 0;
  scopes->scopes[scopes->scopes_count -1].vars = smalloc(scopes->scopes[scopes->scopes_count -1].vars_count * sizeof(*scopes->scopes[scopes->scopes_count -1].vars));
}

// create a copy of the scopes and its variables
static Symbol_table copy_Symbol_table(Symbol_table scopes) {
  Symbol_table result;
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
static void free_Symbol_table(Symbol_table variables) {
  for (int i = 0; i < variables.scopes_count; i++ ) {
    free(variables.scopes[i].vars);
  }
  free(variables.scopes);
}

// check if a statement is valid, if it is not, report it and halt
void check_statement(Symbol_table * variables, const Node_Statement stmt) {
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
        const Token previous_var = get_symbol_from_token(*variables, variable.value).value;
        errorf("Line:%d, column:%d.  Error: variable already declared in line:%d, column:%d.\n", previous_var.line_number, previous_var.column_number, previous_var.line_number, previous_var.column_number);
      }
      else {
        append_var_to_var_list(variable, variables);
      }

      // check that the types of the declaration are valid with the ones of the expresion
      if (!compare_2_types(variable.type, get_type_of_expresion(*variables, expresion))) {
        error("the type in variable declaration does not match the expresion type");
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
        errorf("Line:%d, column:%d.  Error: variable has not been declared before.\n", variable.line_number, variable.column_number);
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
      Symbol_table scope_vars = copy_Symbol_table(*variables);
      for (int i = 0; i < stmt.statement_value.scope.statements_count; i++) {
        check_statement(&scope_vars, stmt.statement_value.scope.statements_node[i]);
      }
      free_Symbol_table(scope_vars);
      break;
    }
    case if_type: {
      Node_Expresion condition = stmt.statement_value.if_node.condition;
      is_expresion_valid(*variables, condition);
      Symbol_table scope_vars = copy_Symbol_table(*variables);
      for (int i = 0; i < stmt.statement_value.if_node.scope.statements_count; i++) {
        check_statement(&scope_vars, stmt.statement_value.if_node.scope.statements_node[i]);
      }
      if (stmt.statement_value.if_node.has_else_block) {
        for (int i = 0; i < stmt.statement_value.if_node.else_block.statements_count; i++) {
          check_statement(&scope_vars, stmt.statement_value.if_node.else_block.statements_node[i]);
        }
      }
      free_Symbol_table(scope_vars);
      break;
    }
    case while_type: {
      Node_Expresion condition = stmt.statement_value.while_node.condition;
      is_expresion_valid(*variables, condition);
      Symbol_table scope_vars = copy_Symbol_table(*variables);
      for (int i = 0; i < stmt.statement_value.while_node.scope.statements_count; i++) {
        check_statement(&scope_vars, stmt.statement_value.while_node.scope.statements_node[i]);
      }
      free_Symbol_table(scope_vars);
      break;
    }
  }
}

// checks if the program follows the grammar rules and the language specifications
bool is_valid_program(Node_Program program) {
  Symbol_table scopes;
  scopes.scopes_count = 0;
  scopes.scopes = malloc(scopes.scopes_count * sizeof(Symbols_scope));
  create_scope(&scopes); // create the first global scope
  // check each statement correctness
  for (int i = 0; i < program.statements_count; i++) {
    Node_Statement statement = program.statements_node[i];
    check_statement(&scopes, statement);
  }
  free_Symbol_table(scopes);
  return true;
}

#endif