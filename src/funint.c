#include <time.h>

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
  printf("code freed\n");
  free(tokens);
  printf("tokens freed\n");
  // free the expresions of the program recursively
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    if (node.statement_type == var_declaration_type) {
      // var declaration node
      free_expresion(node.statement_value.var_declaration.value);
    }
    else if (node.statement_type == exit_node_type) {
      // exit node
      free_expresion(node.statement_value.exit_node.exit_code);
    }
  }
  free(syntax_tree.statements_node);
  printf("syntax tree freed\n");
}

void compile(char * source_code_file, char * result_file) {
  // open the source code file
  FILE * file = fopen(source_code_file, "r");
  char * code = file_contents(file);

  Token * tokens = lexer(code);
  Node_Program syntax_tree = parser(tokens);
  if (is_valid_program(syntax_tree)) {
    printf("program is valid\n");
    gen_C_code(syntax_tree, result_file);
  }
  else {
    error("program is not valid");
  }

  // free all the memory
  free_all_memory(code, tokens, syntax_tree);
}

int main(int argc, char ** argv) {
  if (argc != 3) {
    error("invalid number of cmd arguments, you must write:\n  compiler [input file path] [output file path]");
  }
  // start clock
  clock_t start, end;
  float time;
  start = clock();

  char * code = argv[1];
  char * out_file = argv[2];

  compile(code, out_file);

  // time it
  end = clock();
  time = ((float) (end - start)) / CLOCKS_PER_SEC;
  printf("\n#######\ntime: %f\n", time);

  printf("compilation finnished succesfully\n");

  return 0;
}
