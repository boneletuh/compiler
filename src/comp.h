#ifndef COMP_H_
#define COMP_H_

#include <time.h>
#include <string.h>

#include "mlib.h"
#include "errors.h"
#include "tokenizer.h"
#include "parser.h"
#include "checker.h"
#include "generator.h"


void free_type(Node_Type type) {
  switch (type.type_type) {
    case type_primitive_type: break;
    case type_ptr_type:
      free_type(*type.type_value.type_ptr_value);
      free(type.type_value.type_ptr_value);
      break;
  }
}

void free_expresion(Node_Expresion expresion) {
  switch (expresion.expresion_type) {
    case expresion_binary_operation_type:
      free_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side);
      free_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side);
      free(expresion.expresion_value.expresion_binary_operation_value);
      break;
    case expresion_unary_operation_type:
      free_expresion(expresion.expresion_value.expresion_unary_operation_value->expresion);
      free(expresion.expresion_value.expresion_unary_operation_value);
      break;
    // nothing to free in this
    case expresion_number_type: break;
    case expresion_identifier_type: break;
  }
}

void free_scope(Node_Scope scope);

void free_stmt(Node_Statement stmt) {
    switch (stmt.statement_type) {
      case var_declaration_type:
        free_expresion(stmt.statement_value.var_declaration.value);
        free_type(stmt.statement_value.var_declaration.type);
        break;
      case exit_node_type:
        free_expresion(stmt.statement_value.exit_node.exit_code);
        break;
      case var_assignment_type:
        free_expresion(stmt.statement_value.var_assignment.value);
        break;
      case print_type:
        free_expresion(stmt.statement_value.print.chr);
        break;
      case scope_type:
        free_scope(stmt.statement_value.scope);
        break;
      case if_type:
        free_expresion(stmt.statement_value.if_node.condition);
        free_scope(stmt.statement_value.if_node.scope);
        if (stmt.statement_value.if_node.has_else_block) {
          free_scope(stmt.statement_value.if_node.else_block);
        }
        break;
      case while_type:
        free_expresion(stmt.statement_value.while_node.condition);
        free_scope(stmt.statement_value.while_node.scope);
        break;
    }
}

void free_scope(Node_Scope scope) {
  for (int i = 0; i < scope.statements_count; i++) {
    free_stmt(scope.statements_node[i]);
  }
  free(scope.statements_node);
}

// frees all the allocated memory
void free_all_memory(char * code, Token * tokens, Node_Program syntax_tree) {
  free(code);
  free(tokens);
  // free the expresions of the program recursively
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    free_stmt(node);
  }
  free(syntax_tree.statements_node);
}

void compile(const char * source_code_file, const char * result_file) {
  // open the source code file
  FILE * file = fopen(source_code_file, "r");
  char * code = file_contents(file);

  Token * tokens = lexer(code);

  Node_Program syntax_tree = parser(tokens);

  if (is_valid_program(syntax_tree)) {
    const char * extension = get_file_extension(result_file);
    if (strcmp(extension, ".c") == 0) {
      gen_C_code(syntax_tree, result_file);
    }
    else if (strcmp(extension, ".asm") == 0) {
      gen_NASM_code(syntax_tree, result_file);
    }
    else {
      error("the output file must have a supported file extension");
    }
  }
  else {
    error("program is not valid");
  }
  free_all_memory(code, tokens, syntax_tree);
}

#endif