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
      case scope_type:
        free(node.statement_value.scope.statements_node);
        // FIX: free the memory of the statemets inside the scope
        break;
    }
  }
  free(syntax_tree.statements_node);
  printf("memory freed");
}

void compile(char * source_code_file, char * result_file) {
  // open the source code file
  FILE * file = fopen(source_code_file, "r");
  char * code = file_contents(file);

  Token * tokens = lexer(code);

  Node_Program syntax_tree = parser(tokens);

  if (is_valid_program(syntax_tree)) {
    char * extension = get_file_extension(result_file);
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

int main(int argc, char ** argv) {
  if (argc != 3) {
    error("invalid number of cmd arguments, you must write:\n  compiler [input file path] [output file path]");
  }
  // start clock
  clock_t start = clock();

  // TODO: improve cmd args handling
  char * code = argv[1];
  char * out_file = argv[2];

  compile(code, out_file);

  // time it
  float time = ((float) (clock() - start)) / CLOCKS_PER_SEC;
  printf("\n#######\ntime: %f\n", time);

  printf("compilation ended");

  return 0;
}
