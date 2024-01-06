#ifndef GENERATOR_H_
#define GENERATOR_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"

FILE * create_file(char * file_name) {
  FILE * fptr = fopen(file_name,"w");
  if (fptr == NULL) {
    file_error("could not create the file for the output");
  }
  return fptr; 
}

void add_string_to_file(FILE * file_ptr, char * string) {
  fprintf(file_ptr, string);
}

void gen_C_expresion(Node_Expresion expresion, FILE * file_ptr) {
  if (expresion.expresion_type == expresion_number_type) {
    add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value.number_token);
  }
  else if (expresion.expresion_type == expresion_identifier_type) {
    add_token_to_file(file_ptr, expresion.expresion_value.expresion_identifier_value);
  }
  else if (expresion.expresion_type == expresion_binary_operation_type) {
    add_string_to_file(file_ptr, "(");
    gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side, file_ptr);
    if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_sum_type) {
      add_string_to_file(file_ptr, " + ");
    }
    else if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_sub_type) {
      add_string_to_file(file_ptr, " - ");
    }
    else if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_mul_type) {
      add_string_to_file(file_ptr, " * ");
    }
    else if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_div_type) {
      add_string_to_file(file_ptr, " / ");
    }
    else if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_mod_type) {
      add_string_to_file(file_ptr, " %% ");
    }
    else if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_exp_type) {
      //add_string_to_file(file_ptr, " ^ ");
      implementation_error("exponentation not implemented");
    }
    else {
      implementation_error("ukown operation in expresion");
    }
    gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr);
    add_string_to_file(file_ptr, ")");
  }
  else {
    implementation_error("unexpected type in expresion");
  }
}

// it generates c code
void gen_C_code(Node_Program syntax_tree, char * out_file_name) {
  FILE * out_file_ptr = create_file(out_file_name);
  add_string_to_file(out_file_ptr, "#include <stdlib.h>\nvoid main() {\n");
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    if (node.statement_type == var_declaration_type) {
      // var declaration node
      add_string_to_file(out_file_ptr, " int ");
      add_token_to_file(out_file_ptr, node.statement_value.var_declaration.var_name);
      add_string_to_file(out_file_ptr, " = ");
      gen_C_expresion(node.statement_value.var_declaration.value, out_file_ptr);
    }
    else if (node.statement_type == exit_node_type) {
      // exit node
      add_string_to_file(out_file_ptr, " exit(");
      gen_C_expresion(node.statement_value.exit_node.exit_code, out_file_ptr);
      add_string_to_file(out_file_ptr, ")");
    }
    else {
      implementation_error("undefined node statement type");
    }
    add_string_to_file(out_file_ptr, ";\n");
  }
  add_string_to_file(out_file_ptr, "}");
  fclose(out_file_ptr);
}

#endif