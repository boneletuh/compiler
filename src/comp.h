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


void free_expresion(Node_Expresion expresion) {
  if (expresion.expresion_type == expresion_binary_operation_type) {
    free_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side);
    free_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side);
    free(expresion.expresion_value.expresion_binary_operation_value);
  }
}

// frees all the allocated memory
void free_all_memory(char * code, Token * tokens, Node_Program syntax_tree) {
  free(code);
  free(tokens);
  // free the expresions of the program recursively
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    switch (node.statement_type) {
      case var_declaration_type:
        // var declaration node
        free_expresion(node.statement_value.var_declaration.value);
        break;
      case exit_node_type:
        // exit node
        free_expresion(node.statement_value.exit_node.exit_code);
        break;
      case var_assignment_type:
        free_expresion(node.statement_value.var_assignment.value);
        break;
      case print_type:
        free_expresion(node.statement_value.print.chr);
        break;
      // FIX: handle: scope, if, while
    }
  }
  free(syntax_tree.statements_node);
}

void compile(char * source_code_file, char * result_file) {
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
  // free all the memory ºOº
  free_all_memory(code, tokens, syntax_tree);
}

#endif