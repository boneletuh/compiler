#ifndef GENERATOR_H_
#define GENERATOR_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"


static void add_token_to_file(FILE * file_ptr, const Token token) {
  fprintf(file_ptr, "%.*s", token.length, token.beginning);
}

static void add_string_to_file(FILE * file_ptr, const char * string) {
  fputs(string, file_ptr);
}

// the size of the native types in bytes
enum Types_sizes {
  U64_sz = 8,
  PTR_sz = 8  // only in 64 bit platforms
};

// returns the size of the given type in bytes
static int get_size_of_type(Node_Type type) {
  switch (type.type_type) {
    case type_primitive_type:
      return U64_sz;
      break;
    case type_ptr_type:
      return PTR_sz;
      break;
    case type_array_type:
      return get_size_of_type(*type.type_value.type_array_value->primitive_type) * number_token_to_int(type.type_value.type_array_value->elements_count);
      break;
  }
  implementation_error("tried to get the size of an unkown type");
  // unreachable
  return -1;
}


/* * * * * * * * * * *
 * Generating C code *
 * * * * * * * * * * */

static void gen_C_scope(const Node_Scope scope, FILE * out_file_name);
void gen_C_code(const Node_Program syntax_tree, const char * out_file_name);

static void gen_C_expresion(const Node_Expresion expresion, FILE * file_ptr) {
  switch (expresion.expresion_type) {
    case expresion_number_type:
      add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value);
      break;
    case expresion_identifier_type:
      add_token_to_file(file_ptr, expresion.expresion_value.expresion_identifier_value);
      break;

    case expresion_binary_operation_type:
      add_string_to_file(file_ptr, "(");
      gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side, file_ptr);
      switch (expresion.expresion_value.expresion_binary_operation_value->operation_type) {
        case binary_operation_sum_type:
          add_string_to_file(file_ptr, " + ");
          break;

        case binary_operation_sub_type:
          add_string_to_file(file_ptr, " - ");
          break;

        case binary_operation_mul_type:
          add_string_to_file(file_ptr, " * ");
          break;

        case binary_operation_div_type:
          add_string_to_file(file_ptr, " / ");
          break;

        case binary_operation_mod_type:
          add_string_to_file(file_ptr, " % ");
          break;

        case binary_operation_exp_type:
          implementation_error("exponentation not implemented");
          break;

        case binary_operation_big_type:
          add_string_to_file(file_ptr, " > ");
          break;

        case binary_operation_les_type:
          add_string_to_file(file_ptr, " < ");
          break;

        case binary_operation_equ_type:
          add_string_to_file(file_ptr, " == ");
          break;

        default:
          implementation_error("unknown operation in expresion");
          break;
      }
      gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr);
      add_string_to_file(file_ptr, ")");
      break;

    case expresion_unary_operation_type:
      switch (expresion.expresion_value.expresion_unary_operation_value->operation_type) {
        case unary_operation_addr_type:
          add_string_to_file(file_ptr, "&");
          break;

        case unary_operation_deref_type:
          add_string_to_file(file_ptr, "*");
          break;

        default:
          implementation_error("unknown operation in unary expresion in C code gen");
          break;
      }
      add_string_to_file(file_ptr, "(");
      gen_C_expresion(expresion.expresion_value.expresion_unary_operation_value->expresion, file_ptr);
      add_string_to_file(file_ptr, ")");
      break;

    case expresion_array_type:
      add_string_to_file(file_ptr, "{");
      for (int i = 0; i < expresion.expresion_value.expresion_array_value->elements_count; i++) {
        gen_C_expresion(expresion.expresion_value.expresion_array_value->elements[i], file_ptr);
        add_string_to_file(file_ptr, ", ");
      }
      add_string_to_file(file_ptr, "}");
      break;

    default:
      implementation_error("unexpected type in expresion while C expresion generation");
      break;
  }
}

static void gen_C_type(FILE * out_file_ptr, const Node_Type type) {
  switch (type.type_type) {
    case type_primitive_type:
      add_string_to_file(out_file_ptr, "uint64_t ");
      break;
    case type_ptr_type:
      gen_C_type(out_file_ptr, *type.type_value.type_ptr_value);
      add_string_to_file(out_file_ptr, "* ");
      break;
    case type_array_type:
      gen_C_type(out_file_ptr, *type.type_value.type_array_value->primitive_type);
      add_string_to_file(out_file_ptr, "[");
      add_token_to_file(out_file_ptr, type.type_value.type_array_value->elements_count);
      add_string_to_file(out_file_ptr, "]");
      break;
    default:
      implementation_error("generation of this type of type not implemented in C");
  }
}

static void gen_C_var_decl_type_and_name(FILE * out_file_ptr, const Token var_name, const Node_Type type) {
  bool has_var_name_been_written = false;
  switch (type.type_type) {
    case type_primitive_type:
      add_string_to_file(out_file_ptr, "uint64_t ");
      break;
    case type_ptr_type:
      gen_C_type(out_file_ptr, *type.type_value.type_ptr_value);
      add_string_to_file(out_file_ptr, "* ");
      break;
    case type_array_type:
      // FIX: the recursive arrays translation to C in wrong (sometimes), it needs brackets
      gen_C_type(out_file_ptr, *type.type_value.type_array_value->primitive_type);
        if (!has_var_name_been_written) {
        add_token_to_file(out_file_ptr, var_name);
      }
      has_var_name_been_written = true;

      add_string_to_file(out_file_ptr, "[");
      add_token_to_file(out_file_ptr, type.type_value.type_array_value->elements_count);
      add_string_to_file(out_file_ptr, "]");
      break;
    default:
      implementation_error("generation of this type of type not implemented in C");
  }
  if (!has_var_name_been_written) {
    add_token_to_file(out_file_ptr, var_name);
  }
}

static void gen_C_statement(const Node_Statement stmt, FILE * out_file_ptr) {
  switch (stmt.statement_type) {
    case var_declaration_type:
      // var declaration node
      add_string_to_file(out_file_ptr, " ");
      gen_C_var_decl_type_and_name(out_file_ptr, stmt.statement_value.var_declaration.var_name, stmt.statement_value.var_declaration.type);

      add_string_to_file(out_file_ptr, " = ");
      gen_C_expresion(stmt.statement_value.var_declaration.value, out_file_ptr);
      break;

    case exit_node_type:
      // exit node
      add_string_to_file(out_file_ptr, " exit((uint64_t)");
      gen_C_expresion(stmt.statement_value.exit_node.exit_code, out_file_ptr);
      add_string_to_file(out_file_ptr, ")");
      break;

    case print_type:
      // print node
      add_string_to_file(out_file_ptr, " putchar(");
      gen_C_expresion(stmt.statement_value.print.chr, out_file_ptr);
      add_string_to_file(out_file_ptr, "&0xff)");
      break;

    case var_assignment_type:
      // var assignment node
      add_string_to_file(out_file_ptr, " ");
      add_token_to_file(out_file_ptr, stmt.statement_value.var_assignment.var_name);
      add_string_to_file(out_file_ptr, " = ");
      gen_C_expresion(stmt.statement_value.var_assignment.value, out_file_ptr);
      break;

    case scope_type:
      // scope node
      add_string_to_file(out_file_ptr, " {\n");
      gen_C_scope(stmt.statement_value.scope, out_file_ptr);
      add_string_to_file(out_file_ptr, " }");
      break;
    
    case if_type:
      // if node
      // generate the condition
      add_string_to_file(out_file_ptr, " if ( ");
      gen_C_expresion(stmt.statement_value.if_node.condition, out_file_ptr);
      // generate the scope
      add_string_to_file(out_file_ptr, " ) {\n");
      gen_C_scope(stmt.statement_value.if_node.scope, out_file_ptr);
      add_string_to_file(out_file_ptr, " }");
      if (stmt.statement_value.if_node.has_else_block) {
        add_string_to_file(out_file_ptr, " else {\n");
        gen_C_scope(stmt.statement_value.if_node.else_block, out_file_ptr);
        add_string_to_file(out_file_ptr, "}");
      }
      break;

    case while_type:
      // if node
      // generate the condition
      add_string_to_file(out_file_ptr, " while ( ");
      gen_C_expresion(stmt.statement_value.while_node.condition, out_file_ptr);
      // generate the scope
      add_string_to_file(out_file_ptr, " ) {\n");
      gen_C_scope(stmt.statement_value.while_node.scope, out_file_ptr);
      add_string_to_file(out_file_ptr, " }");
      break;

    default:
      implementation_error("undefined node statement type");
      break;
  }
  add_string_to_file(out_file_ptr, ";\n");
}

static void gen_C_scope(const Node_Scope scope, FILE * out_file_ptr) {
  for (int i = 0; i < scope.statements_count; i++) {
    gen_C_statement(scope.statements_node[i], out_file_ptr);
  }
}

// it generates C code
void gen_C_code(const Node_Program syntax_tree, const char * out_file_name) {
  FILE * out_file_ptr = create_file(out_file_name);
  add_string_to_file(out_file_ptr, "#include <stdlib.h>\n#include <stdio.h>\n#include <stdint.h>\nint main() {\n");
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    gen_C_statement(node, out_file_ptr);
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

typedef struct Scopes_List {
  int scopes_count;
  Variables_List * variables;
} Scopes_List;

// find the place in where the variable is located in the stack
static int find_var_stack_place(const Scopes_List vars_list, const Token var) {
  for (int i = 0; i < vars_list.scopes_count; i++) {
    for (int j = 0; j < vars_list.variables[i].var_stack_size; j++) {
      if (compare_str_of_tokens(var, vars_list.variables[i].var_stack_tokens_list[j])) {
        return vars_list.variables[i].var_stack_places_list[j];
      }
    }
  }
  // couldnt find the variable in the list
  // this sould have been detected by the checker
  implementation_error("could not find variable in variable list");
  // unreachable
  return -1;
}

// generates asm from expresion the result will be put in the top of the stack
static void gen_NASM_expresion(FILE * file_ptr, const Node_Expresion expresion, int stack_size, const Scopes_List vars) {
  switch (expresion.expresion_type) {
    case expresion_number_type:
      stack_size += U64_sz;
      add_string_to_file(file_ptr, "mov qword [rbp - ");
      fprintf(file_ptr, "%d", stack_size);
      add_string_to_file(file_ptr, "], ");
      add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value);
      add_string_to_file(file_ptr, "\n");
      break;

    case expresion_identifier_type:
      stack_size += 8;
      // put variable into 'rax'
      add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
      fprintf(file_ptr, "%d", find_var_stack_place(vars, expresion.expresion_value.expresion_identifier_value));
      add_string_to_file(file_ptr, "]\n");
      // put rax onto the stack
      add_string_to_file(file_ptr, "mov qword [rbp - ");
      fprintf(file_ptr, "%d", stack_size);
      add_string_to_file(file_ptr, "], rax\n");
      break;

    case expresion_binary_operation_type:
      // put left hand side expresion into stack top
      stack_size += 8;
      gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->left_side, stack_size, vars);
      // put also the right side into the stack
      stack_size += 8;
      gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->right_side, stack_size, vars);
      // load the first operand
      add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
      fprintf(file_ptr, "%d", stack_size);
      add_string_to_file(file_ptr, "]\n");
      // load the second operand
      add_string_to_file(file_ptr, "mov rbx, qword [rbp - ");
      fprintf(file_ptr, "%d", stack_size+8);
      add_string_to_file(file_ptr, "]\n");
      // perform the corresponding binary operation
      switch (expresion.expresion_value.expresion_binary_operation_value->operation_type) {
        case binary_operation_sum_type:
          // add them together
          add_string_to_file(file_ptr, "add rax, rbx\n");
          // put the result into the stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        case binary_operation_sub_type:
          // substruct them
          add_string_to_file(file_ptr, "sub rax, rbx\n");
          // put the result in stack topo
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        case binary_operation_mul_type:
          // multiply them
          add_string_to_file(file_ptr, "mul rbx\n"); // NOTE: this changes rdx
          // put the result in stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        case binary_operation_div_type:
          // set rdx to 0 because the dividend is the extended register rdx:rax
          add_string_to_file(file_ptr, "xor rdx, rdx\n");
          // divide them
          add_string_to_file(file_ptr, "div rbx\n");
          // put the result in stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        case binary_operation_mod_type:
          // set rdx to 0 because the 'dividend' is the extended register rdx:rax
          add_string_to_file(file_ptr, "xor rdx, rdx\n");
          // perform the modulo
          add_string_to_file(file_ptr, "div rbx\n");
          // put the result in stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rdx\n");
          break;

        case binary_operation_equ_type:
          // if rax and rbx are equal set rax to 1 otherwise to 0
          add_string_to_file(file_ptr, "cmp rax, rbx\n");
          add_string_to_file(file_ptr, "sete al\n");
          add_string_to_file(file_ptr, "movzx rax, al\n");
          // put the result in stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        case binary_operation_big_type:
          // if rax and rbx are equal set rax to 1 otherwise to 0
          add_string_to_file(file_ptr, "cmp rax, rbx\n");
          add_string_to_file(file_ptr, "seta al\n");
          add_string_to_file(file_ptr, "movzx rax, al\n");
          // put the result in stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;
          
        case binary_operation_les_type:
          // if rax and rbx are equal set rax to 1 otherwise to 0
          add_string_to_file(file_ptr, "cmp rax, rbx\n");
          add_string_to_file(file_ptr, "setb al\n");
          add_string_to_file(file_ptr, "movzx rax, al\n");
          // put the result in stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-8);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        // TODO: add the ^ operator
        default:
          // this operation has not been added yet or was not filtered by the checker
          implementation_error("unkown operation type while code generation");
      }
      break;

    case expresion_unary_operation_type:
      // perform the corresponding unary operation
      switch (expresion.expresion_value.expresion_unary_operation_value->operation_type) {
        case unary_operation_addr_type:
          // get the address of a variable
          stack_size += PTR_sz;
          int var_addr =  find_var_stack_place(vars, expresion.expresion_value.expresion_unary_operation_value->expresion.expresion_value.expresion_identifier_value);
          add_string_to_file(file_ptr, "lea rax, [rbp - ");
          fprintf(file_ptr, "%d", var_addr);
          add_string_to_file(file_ptr, "]\n");
          // put the result into the stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size);
          add_string_to_file(file_ptr, "], rax\n");
          break;
        case unary_operation_deref_type:
          stack_size += PTR_sz;
          // generate the expression
          gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_unary_operation_value->expresion, stack_size, vars);
          // dereference the pointer
          add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size-PTR_sz);
          add_string_to_file(file_ptr, "]\n");
          add_string_to_file(file_ptr, "mov rax, qword [rax]\n");
          // put the result into the stack top
          add_string_to_file(file_ptr, "mov qword [rbp - ");
          fprintf(file_ptr, "%d", stack_size);
          add_string_to_file(file_ptr, "], rax\n");
          break;

        default:
          // this operation has not been added yet or was not filtered by the checker
          implementation_error("unkown unary operation type while ASM code generation");
      }
      break;

  case expresion_array_type:;
    Node_Array array = *expresion.expresion_value.expresion_array_value;
    // FIX: get the size of the individual elements of the array using a function
    int single_element_size = U64_sz; //get_size_of_expresion(array.elements[0]);
    // generate every element in the array
    for (int i = 0; i < array.elements_count; i++) {
      gen_NASM_expresion(file_ptr, array.elements[i], stack_size, vars);
      // progresively allocate space for the element in each iteration
      stack_size += single_element_size;
    }
    break;

  default:
    implementation_error("unexpected type in expresion while ASM binary operation generation");
    break;
  }
}

static Scopes_List NASM_copy_scopes_list(const Scopes_List scopes) {
  Scopes_List result;
  result.scopes_count = scopes.scopes_count;
  result.variables = smalloc(scopes.scopes_count * sizeof(*scopes.variables));
  for (int i = 0; i < scopes.scopes_count; i++) {
    result.variables[i].var_stack_size = scopes.variables[i].var_stack_size;
    result.variables[i].var_stack_places_list = smalloc(result.variables[i].var_stack_size * sizeof(*result.variables[i].var_stack_places_list));
    memcpy(result.variables[i].var_stack_places_list, scopes.variables[i].var_stack_places_list, result.variables[i].var_stack_size * sizeof(*result.variables[i].var_stack_places_list));

    result.variables[i].var_stack_tokens_list = smalloc(result.variables[i].var_stack_size * sizeof(*result.variables[i].var_stack_tokens_list));
    memcpy(result.variables[i].var_stack_tokens_list, scopes.variables[i].var_stack_tokens_list, result.variables[i].var_stack_size * sizeof(*result.variables[i].var_stack_tokens_list));
  }
  return result;
}

// add a variable to the last scope of list of variables, and its place on the stack
static void NASM_append_var_to_var_list(Scopes_List * scopes, const Token variable, const int stack_place) {
  Variables_List * last_scope = &scopes->variables[scopes->scopes_count -1];
  last_scope->var_stack_size++;
  last_scope->var_stack_places_list = srealloc(last_scope->var_stack_places_list, last_scope->var_stack_size * sizeof(*last_scope->var_stack_places_list));
  last_scope->var_stack_places_list[last_scope->var_stack_size -1] = stack_place;
  last_scope->var_stack_tokens_list = srealloc(last_scope->var_stack_tokens_list, last_scope->var_stack_size * sizeof(*last_scope->var_stack_tokens_list));
  last_scope->var_stack_tokens_list[last_scope->var_stack_size -1] = variable;
}

// create a new scope and append it to the end of the list of scopes
static void NASM_create_scope(Scopes_List * scopes) {
  scopes->scopes_count++;
  scopes->variables = srealloc(scopes->variables, scopes->scopes_count * sizeof(*scopes->variables));
  scopes->variables[scopes->scopes_count -1].var_stack_size = 0;
  scopes->variables[scopes->scopes_count -1].var_stack_tokens_list = malloc(scopes->variables[scopes->scopes_count -1].var_stack_size * sizeof(Token));
  scopes->variables[scopes->scopes_count -1].var_stack_places_list = malloc(scopes->variables[scopes->scopes_count -1].var_stack_size * sizeof(int));
}

static void free_scopes_list(Scopes_List scopes) {
  for (int i = 0; i < scopes.scopes_count; i++) {
    free(scopes.variables[i].var_stack_places_list);
    free(scopes.variables[i].var_stack_tokens_list);
  }
  free(scopes.variables);
}

// keep track of an unique identification for the labels so there arent collisions with other labels
static int uuid = 0;


static void gen_NASM_statement(FILE * out_file_ptr, Scopes_List * variables, const Node_Statement stmt, int * stack_size);


static void gen_NASM_var_declaration(FILE * out_file_ptr, Scopes_List * variables, Node_Var_declaration var_declaration, int * stack_size) {
  if (var_declaration.type.type_type == type_array_type) {
    int array_size = get_size_of_type(var_declaration.type);
    gen_NASM_expresion(out_file_ptr, var_declaration.value, *stack_size, *variables);
    // allocate space for array in stack
    *stack_size += array_size;
    // add the variable to the list of vars
    NASM_append_var_to_var_list(variables, var_declaration.var_name, *stack_size);
  }
  else {
    int expr_size = get_size_of_type(var_declaration.type);
    gen_NASM_expresion(out_file_ptr, var_declaration.value, *stack_size, *variables);
    // get the variable from the top of the stack into rax
    add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
    fprintf(out_file_ptr, "%d", *stack_size + expr_size); // add the size because the value is above the stack top
    add_string_to_file(out_file_ptr, "]\n");
    // allocate space for variable in stack
    *stack_size += expr_size;
    // assign the value of the expresion from the stack
    add_string_to_file(out_file_ptr, "mov qword [rbp - ");
    fprintf(out_file_ptr, "%d", *stack_size);
    add_string_to_file(out_file_ptr, "], rax"); // rax has the result of the expresion
    // add the variable to the list of vars
    NASM_append_var_to_var_list(variables, var_declaration.var_name, *stack_size);
  }
}

static void gen_NASM_exit_node(FILE * out_file_ptr, Scopes_List * variables, Node_Exit exit_node, int * stack_size) {
  // NOTE: this only works for unix-like OSes
  gen_NASM_expresion(out_file_ptr, exit_node.exit_code, *stack_size, *variables);
  add_string_to_file(out_file_ptr, "mov rax, 60\n");
  add_string_to_file(out_file_ptr, "mov rdi, qword [rbp - ");
  fprintf(out_file_ptr, "%d", *stack_size+U64_sz);
  add_string_to_file(out_file_ptr, "]\n");
  add_string_to_file(out_file_ptr, "syscall\n");
}

static void gen_NASM_scope(FILE * out_file_ptr, Scopes_List * variables, Node_Scope scope, int * stack_size) {
  Scopes_List temp_scopes = NASM_copy_scopes_list(*variables); // FIX: create new scope instead of copying all the vars
  //NASM_create_scope(&temp_scopes);
  int temp_stack_size = *stack_size;
  for (int i = 0; i < scope.statements_count; i++) {
    gen_NASM_statement(out_file_ptr, &temp_scopes, scope.statements_node[i], &temp_stack_size);
  }
  free_scopes_list(temp_scopes);
}

static void gen_NASM_var_assignment(FILE * out_file_ptr, Scopes_List * variables, Node_Var_assignment var_assignment, int * stack_size) {
  gen_NASM_expresion(out_file_ptr, var_assignment.value, *stack_size, *variables);
  // get the variable from the top of the stack into rax
  add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
  fprintf(out_file_ptr, "%d", *stack_size+8); // add 8 because the value is above the stack top
  add_string_to_file(out_file_ptr, "]\n");
  // assign the value of the expresion from the stack
  add_string_to_file(out_file_ptr, "mov qword [rbp - ");
  fprintf(out_file_ptr, "%d", find_var_stack_place(*variables, var_assignment.var_name));
  add_string_to_file(out_file_ptr, "], rax"); // rax has the result of the expresion
}

static void gen_NASM_print(FILE * out_file_ptr, Scopes_List * variables, Node_Print print_node, int * stack_size) {
  // NOTE: this only works for unix-like OSes
  gen_NASM_expresion(out_file_ptr, print_node.chr, *stack_size, *variables);
  add_string_to_file(out_file_ptr, "lea rsi, [rbp - ");
  fprintf(out_file_ptr, "%d", *stack_size+U64_sz);
  add_string_to_file(out_file_ptr, "]\n");
  add_string_to_file(out_file_ptr, "mov rdx, 1\n"); // symbols to print
  add_string_to_file(out_file_ptr, "mov rdi, 1\n"); // std output
  add_string_to_file(out_file_ptr, "mov rax, 1\n"); // write syscall
  add_string_to_file(out_file_ptr, "syscall\n");
}

static void gen_NASM_if_node(FILE * out_file_ptr, Scopes_List * variables, Node_If if_node, int * stack_size) {
  // generate the condition
  Node_Expresion condition = if_node.condition;
  gen_NASM_expresion(out_file_ptr, condition, *stack_size, *variables);

  // if the condition is not true skip the if body
  add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
  fprintf(out_file_ptr, "%d", *stack_size+U64_sz);
  add_string_to_file(out_file_ptr, "]\n");
  add_string_to_file(out_file_ptr, "test rax, rax\n");
  int if_uid = uuid; // save the uid in case it gets modified in the scope
  uuid++;
  // if the condition is not met skip the `if` block
  fprintf(out_file_ptr, "jz .IF%d\n", if_uid);

  { // generate the `if` scope
    Scopes_List temp_scopes = NASM_copy_scopes_list(*variables);
    //NASM_create_scope(&temp_scopes);
    int tmp_stack_sz = *stack_size;
    for (int i = 0; i < if_node.scope.statements_count; i++) {
      gen_NASM_statement(out_file_ptr, &temp_scopes, if_node.scope.statements_node[i], &tmp_stack_sz);
    }
    free_scopes_list(temp_scopes);
  }
  if (if_node.has_else_block) {
    // if the `if` block is executed skip the `else` block
    fprintf(out_file_ptr, "jmp .EL%d\n", if_uid);
  }
  // generate the `if` label
  fprintf(out_file_ptr, ".IF%d:\n", if_uid);
  if (if_node.has_else_block) {
    // generate `else` block code
    Scopes_List temp_scopes = NASM_copy_scopes_list(*variables);
    //NASM_create_scope(&temp_scopes);
    int tmp_stack_sz = *stack_size;
    for (int i = 0; i < if_node.else_block.statements_count; i++) {
      gen_NASM_statement(out_file_ptr, &temp_scopes, if_node.else_block.statements_node[i], &tmp_stack_sz);
    }
    free_scopes_list(temp_scopes);
    fprintf(out_file_ptr, ".EL%d:\n", if_uid); // generate the `else` label
  }
}

static void gen_NASM_while_node(FILE * out_file_ptr, Scopes_List * variables, Node_While while_node, int * stack_size) {
  int while_uid = uuid; // save the uid in case it gets modified in the scope
  uuid++;
  // generate the label for repeating the loop
  fprintf(out_file_ptr, ".WHB%d:\n", while_uid); // WHB is for "while beginning"
  // generate the condition
  Node_Expresion condition = while_node.condition;
  gen_NASM_expresion(out_file_ptr, condition, *stack_size, *variables);

  // if the condition is not true skip the while body
  add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
  fprintf(out_file_ptr, "%d", *stack_size+U64_sz);
  add_string_to_file(out_file_ptr, "]\n");
  add_string_to_file(out_file_ptr, "test rax, rax\n");
  fprintf(out_file_ptr, "jz .WHE%d\n", while_uid); // WHE is for "while end"

  // generate the scope
  Scopes_List temp_scopes = NASM_copy_scopes_list(*variables);
  //NASM_create_scope(&temp_scopes);
  int temp_stack_size = *stack_size;
  for (int i = 0; i < while_node.scope.statements_count; i++) {
    gen_NASM_statement(out_file_ptr, &temp_scopes, while_node.scope.statements_node[i], &temp_stack_size);
  }
  free_scopes_list(temp_scopes);

  fprintf(out_file_ptr, "jmp .WHB%d\n", while_uid);
  fprintf(out_file_ptr, ".WHE%d:\n", while_uid); // generate the label for finnishing the while loop
}


static void gen_NASM_statement(FILE * out_file_ptr, Scopes_List * variables, const Node_Statement stmt, int * stack_size) {
  switch (stmt.statement_type) {
    case var_declaration_type:
      Node_Var_declaration var_declaration = stmt.statement_value.var_declaration;
      gen_NASM_var_declaration(out_file_ptr, variables, var_declaration, stack_size);
      break;

    case exit_node_type:
      Node_Exit exit_node = stmt.statement_value.exit_node;
      gen_NASM_exit_node(out_file_ptr, variables, exit_node, stack_size);
      break;

    case var_assignment_type:
      Node_Var_assignment var_assignment = stmt.statement_value.var_assignment;
      gen_NASM_var_assignment(out_file_ptr, variables, var_assignment, stack_size);
      break;

    case scope_type:
      Node_Scope scope = stmt.statement_value.scope;
      gen_NASM_scope(out_file_ptr, variables, scope, stack_size);
      break;

    case if_type:
      Node_If if_node = stmt.statement_value.if_node;
      gen_NASM_if_node(out_file_ptr, variables, if_node, stack_size);
      break;

    case while_type:
      Node_While while_node = stmt.statement_value.while_node;
      gen_NASM_while_node(out_file_ptr, variables, while_node, stack_size);
      break;
    
    case print_type:
      Node_Print print_node = stmt.statement_value.print;
      gen_NASM_print(out_file_ptr, variables, print_node, stack_size);
      break;

    default:
      implementation_error("undefined node statement type");
      break;
  }
  add_string_to_file(out_file_ptr, "\n\n");
}

// it generates NASM code
void gen_NASM_code(const Node_Program syntax_tree, const char * out_file_name) {
  FILE * out_file_ptr = create_file(out_file_name);
  add_string_to_file(out_file_ptr, "bits 64\n"); // targeting 64 bits
  add_string_to_file(out_file_ptr, "default rel\n"); // make all the pointers `rip` based
  add_string_to_file(out_file_ptr, "global _start\n"); // needed for linking in ELF format
  add_string_to_file(out_file_ptr, "_start:\n");
  add_string_to_file(out_file_ptr, "push rbp\n"); // setting up the stack
  add_string_to_file(out_file_ptr, "mov rbp, rsp\n\n");
  int stack_size = 0;

  // this will hold all the variables from all the scopes
  Scopes_List scopes;
  scopes.scopes_count = 0;
  scopes.variables = malloc(scopes.scopes_count * sizeof(Scopes_List));
  NASM_create_scope(&scopes); // create first global scope

  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    gen_NASM_statement(out_file_ptr, &scopes, node, &stack_size);
  }

  free_scopes_list(scopes);

  // exit the program savely with a syscall
  // NOTE: OS dependent
  add_string_to_file(out_file_ptr, "mov rax, 60\n");
  add_string_to_file(out_file_ptr, "xor rdi, rdi\n");
  add_string_to_file(out_file_ptr, "syscall\n");

  fclose(out_file_ptr);
}


#endif