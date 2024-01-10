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

void add_token_to_file(FILE * file_ptr, Token token) {
  fprintf(file_ptr, "%.*s", token.length, token.beginning);
}

void add_string_to_file(FILE * file_ptr, char * string) {
  fprintf(file_ptr, string);
}

/* * * * * * * * * * *
 * Generating C code *
 * * * * * * * * * * */
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

// it generates C code
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


/* * * * * * * * * * * *
 * Generating ASM code *
 * * * * * * * * * * * */

// type containning the variables of the program and their places in the stack
typedef struct Variables_List {
  int var_stack_size;
  int * var_stack_places_list;
  Token * var_stack_tokens_list;
} Variables_List;

int find_var_stack_place(Variables_List vars_list, Token var) {
  for (int i = 0; i < vars_list.var_stack_size; i++) {
    if (compare_str_of_tokens(var, vars_list.var_stack_tokens_list[i])) {
      return vars_list.var_stack_places_list[i];
    }
  }
  // couldnt find the variable in the list
  // this sould have been detected by the checker 
  implementation_error("could not find variable in variable array");
}

// generates asm from expresion the result will be put in the top of the stack
void gen_NASM_expresion(FILE * file_ptr, Node_Expresion expresion, int stack_size, Variables_List vars) {
  if (expresion.expresion_type == expresion_number_type) {
    stack_size += 8;
    add_string_to_file(file_ptr, "mov QWORD [rbp - ");
    fprintf(file_ptr, "%d", stack_size);
    add_string_to_file(file_ptr, "], ");
    add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value.number_token);
    add_string_to_file(file_ptr, "\n");
  }
  else if (expresion.expresion_type == expresion_identifier_type) {
    stack_size += 8;
    // put variable into 'rax'
    add_string_to_file(file_ptr, "mov rax, QWORD [rbp - ");
    fprintf(file_ptr, "%d", find_var_stack_place(vars, expresion.expresion_value.expresion_identifier_value));
    add_string_to_file(file_ptr, "]\n");
    // put rax onto the stack
    add_string_to_file(file_ptr, "mov QWORD [rbp - ");
    fprintf(file_ptr, "%d", stack_size);
    add_string_to_file(file_ptr, "], rax\n");
  }
  else if (expresion.expresion_type == expresion_binary_operation_type) {
    // put left hand side expresion into stack top
    stack_size += 8;
    gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->left_side, stack_size, vars);
    // put also the right side into the stack
    stack_size += 8;
    gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->right_side, stack_size, vars);
    // load the first operand
    add_string_to_file(file_ptr, "mov rax, QWORD [rbp - ");
    fprintf(file_ptr, "%d", stack_size);
    add_string_to_file(file_ptr, "]\n");
    // load the second operand
    add_string_to_file(file_ptr, "mov rbx, QWORD [rbp - ");
    fprintf(file_ptr, "%d", stack_size+8);
    add_string_to_file(file_ptr, "]\n");
    // perform the corresponding binary operation
    switch (expresion.expresion_value.expresion_binary_operation_value->operation_type) {
      case binary_operation_sum_type:
        // add them together
        add_string_to_file(file_ptr, "add rax, rbx\n");
        // put the result into the stack top
        add_string_to_file(file_ptr, "mov QWORD [rbp - ");
        fprintf(file_ptr, "%d", stack_size-8);
        add_string_to_file(file_ptr, "], rax\n");
        break;
      case binary_operation_sub_type:
        // substruct them
        add_string_to_file(file_ptr, "sub rax, rbx\n");
        // put the result in stack topo
        add_string_to_file(file_ptr, "mov QWORD [rbp - ");
        fprintf(file_ptr, "%d", stack_size-8);
        add_string_to_file(file_ptr, "], rax\n");
        break;
      case binary_operation_mul_type:
        // multiply them
        add_string_to_file(file_ptr, "mul rbx\n"); // NOTE: this changes rdx
        // put the result in stack top
        add_string_to_file(file_ptr, "mov QWORD [rbp - ");
        fprintf(file_ptr, "%d", stack_size-8);
        add_string_to_file(file_ptr, "], rax\n");
        break;
      case binary_operation_div_type:
        // set rdx to 0 because the dividend is the extended register rdx:rax
        add_string_to_file(file_ptr, "mov rdx, 0\n");
        // divide them
        add_string_to_file(file_ptr, "div rbx\n");
        // put the result in stack top
        add_string_to_file(file_ptr, "mov QWORD [rbp - ");
        fprintf(file_ptr, "%d", stack_size-8);
        add_string_to_file(file_ptr, "], rax\n");
        break;
      case binary_operation_mod_type:
        // set rdx to 0 because the 'dividend' is the extended register rdx:rax
        add_string_to_file(file_ptr, "mov rdx, 0\n");
        // perform the modulo
        add_string_to_file(file_ptr, "div rbx\n");
        // put the result in stack top
        add_string_to_file(file_ptr, "mov QWORD [rbp - ");
        fprintf(file_ptr, "%d", stack_size-8);
        add_string_to_file(file_ptr, "], rdx\n");
        break;
      // TODO: add the ^ operator
      default:
        // this operation has not been added yet or was not filtered by the checker
        implementation_error("unkown operation type while code generation");
    }
  }
  else {
    implementation_error("unexpected type in expresion");
  }
}

// it generates NASM code
void gen_NASM_code(Node_Program syntax_tree, char * out_file_name) {
  FILE * out_file_ptr = create_file(out_file_name);
  add_string_to_file(out_file_ptr, "bits 64\n\n");
  int stack_size = 0;
  Variables_List variables;
  variables.var_stack_size = 0;
  variables.var_stack_places_list = malloc(variables.var_stack_size * sizeof(int));
  variables.var_stack_tokens_list = malloc(variables.var_stack_size * sizeof(Token));
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    if (node.statement_type == var_declaration_type) {
      gen_NASM_expresion(out_file_ptr, node.statement_value.var_declaration.value, stack_size, variables);
      // get the variable from the top of the stack into rax
      add_string_to_file(out_file_ptr, "mov rax, QWORD [rbp - ");
      fprintf(out_file_ptr, "%d", stack_size+8); // add 8 becuase the value is above the stack top
      add_string_to_file(out_file_ptr, "]\n");
      // allocate space for variable in stack
      stack_size += 8; // 8 because the size of 64 bit integer
      // assign the value of the expresion from the stack
      add_string_to_file(out_file_ptr, "mov QWORD [rbp - ");
      fprintf(out_file_ptr, "%d", stack_size);
      add_string_to_file(out_file_ptr, "], rax"); // rax has the result of the expresion
      // add the variable to the list of vars
      variables.var_stack_size++;
      variables.var_stack_tokens_list = srealloc(variables.var_stack_tokens_list, variables.var_stack_size * sizeof(*variables.var_stack_tokens_list));
      variables.var_stack_tokens_list[variables.var_stack_size -1] = node.statement_value.var_declaration.var_name;
      // add the place in the stack of the variable to the list
      variables.var_stack_places_list = srealloc(variables.var_stack_places_list, variables.var_stack_size * sizeof(*variables.var_stack_places_list));
      variables.var_stack_places_list[variables.var_stack_size -1] = stack_size;
    }
    else if (node.statement_type == exit_node_type) {
      implementation_error("exit, need syscall here");
    }
    else {
      implementation_error("undefined node statement type");
    }
    add_string_to_file(out_file_ptr, "\n\n");
  }
  free(variables.var_stack_places_list);
  free(variables.var_stack_tokens_list);
  fclose(out_file_ptr);
}


#endif