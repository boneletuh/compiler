#ifndef PARSER_H_
#define PARSER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"

typedef struct Node_Number {
  Token number_token;
} Node_Number;

typedef struct Node_Expresion {
  enum {
    expresion_number_type,
    expresion_identifier_type,
    expresion_binary_operation_type
  } expresion_type;
  union {
    Node_Number expresion_number_value;
    Token expresion_identifier_value;
    // binary operation is a pointer so it can be recursive
    struct Node_Binary_Operation * expresion_binary_operation_value;
  } expresion_value;
} Node_Expresion;

typedef struct Node_Binary_Operation {
  Node_Expresion left_side;
  enum {
    binary_operation_sum_type,
    binary_operation_sub_type,
    binary_operation_mul_type,
    binary_operation_div_type,
    binary_operation_mod_type,
    binary_operation_exp_type
  } operation_type;
  Node_Expresion right_side;
} Node_Binary_Operation;

typedef struct Node_Exit {
  Node_Expresion exit_code;
} Node_Exit;

typedef struct Node_Var_declaration {
  Token var_name;
  Node_Expresion value;
} Node_Var_declaration;

typedef struct Node_Statement {
  union {
    Node_Var_declaration var_declaration;
    Node_Exit exit_node;
  } statement_value;
  enum {
    var_declaration_type,
    exit_node_type
  } statement_type;
} Node_Statement;

typedef struct Node_Program {
  Node_Statement * statements_node;
  int statements_count;
} Node_Program;

// convert the string of a operation token into a enum that is more manageable form
// TODO: see if using a hashtable would be faster 
enum {operation_type} get_operation_type(Token operation) {
  enum {operation_type} type;
  if (compare_token_to_string(operation, "+")) {
    type = binary_operation_sum_type;
  }
  else if (compare_token_to_string(operation, "-")) {
    type = binary_operation_sub_type;
  }
  else if (compare_token_to_string(operation, "*")) {
    type = binary_operation_mul_type;
  }
  else if (compare_token_to_string(operation, "/")) {
    type = binary_operation_div_type;
  }
  else if (compare_token_to_string(operation, "%")) {
    type = binary_operation_mod_type;
  }
  else if (compare_token_to_string(operation, "^")) {
    type = binary_operation_exp_type;
  }
  else {
    error("unkown operation in expresion");
  }
  return type;
}

// get the precedence of an operator acording to the documentation
// the closest the value to 0 the less the precedence is
int get_operation_precedence(Token operation) {
  const char * opers[] = {"+", "-", "%", "*", "/", "^"};
  const int precedes[] = { 0 ,  0 ,  1 ,  2 ,  2 ,  3 };
  // find the idx of the matching string and return the corresponding precedence
  for (int i = 0; i < sizeof(opers)/sizeof(*opers); i++) {
    if (compare_token_to_string(operation, opers[i])) {
      return precedes[i];
    }
  }
  error("unkown precedence of operation");
}

// parses expresion recursively
Node_Expresion parse_expresion(Token * expresion_beginning, int size) {
  Node_Expresion result;
  if (size == 1) {
    // set the type of the expresion
    if (expresion_beginning->type == Number) {
      result.expresion_type = expresion_number_type;
      result.expresion_value.expresion_number_value.number_token = *expresion_beginning;
    }
    else if (expresion_beginning->type == Identifier) {
      result.expresion_type = expresion_identifier_type;
      result.expresion_value.expresion_identifier_value = *expresion_beginning;
    }
    else {
      error("unexpected type in expresion");
    }
  }
  else if (size % 2 == 1) {
    int left_side_beginning = 0;
    int left_side_size;
    Token operation;
    int min_oper_preced = 9999; // some near positive infinity number
    int right_side_beginning;
    int right_side_size;
    // find the beginning and size of the left and right side of the expresion
    int i;
    for (i = 0; i < size; i++) {
      // find the operation with the lowest precedence
      if (expresion_beginning[i].type == Operation) {
          if (get_operation_precedence(expresion_beginning[i]) < min_oper_preced) {
          min_oper_preced = get_operation_precedence(expresion_beginning[i]);
          left_side_size = i;
          operation = expresion_beginning[i];
          right_side_beginning = i +1;
        }
      }
    }
    right_side_size = i - right_side_beginning;
    // create a new node a parse each side of the expresion
    Node_Binary_Operation * bin_operation = smalloc(sizeof(Node_Binary_Operation));
    // parse each side of the binary expresion recursively
    bin_operation->left_side = parse_expresion(&expresion_beginning[left_side_beginning], left_side_size);
    bin_operation->operation_type = get_operation_type(operation);
    bin_operation->right_side = parse_expresion(&expresion_beginning[right_side_beginning], right_side_size);

    result.expresion_value.expresion_binary_operation_value = bin_operation;
    result.expresion_type = expresion_binary_operation_type;
  }
  else {
    implementation_error("parsing this expresion is not implemented");
  }
  return result;
}

// parses the tokens into a syntax tree
Node_Program parser(Token * tokens) {
  Node_Program result_tree;
  int statements_num = 0;
  result_tree.statements_node = malloc(statements_num * sizeof(Node_Statement));
  result_tree.statements_count = 0;

  for (int i = 0; tokens[i].type != End_of_file; i++) {
    statements_num += 1;
    Node_Statement * new_tree = srealloc(result_tree.statements_node, statements_num * sizeof(Node_Statement));
    // exit node
    if (compare_token_to_string(tokens[i], "exit")) {
      // get the number of tokens in the expresion, we add 1 to skip the "exit"
      int expresion_beginning = i +1;
      // FIX: improve this loop, it is very unsafe
      while (tokens[i].type != Semi_colon) i++; 
      int expresion_size = i - expresion_beginning;
      new_tree[statements_num -1].statement_type = exit_node_type;
      new_tree[statements_num -1].statement_value.exit_node.exit_code = parse_expresion(&tokens[expresion_beginning], expresion_size);
      result_tree.statements_node = new_tree;
    }
    // FIX: this would crash if there is an identifier at the end of the file
    else if (compare_token_to_string(tokens[i + 1], ":") && compare_token_to_string(tokens[i + 2], "=")) {
      if (tokens[i].type != Identifier) {
        error("expected an identifier in variable assigment");
      }
      Token var_name = tokens[i];
      // get the number of tokens in the expresion, we add 3 to skip the var name, the ":" and the "="
      int expresion_beginning = i +3;
      // FIX: improve this loop, it is very unsafe
      while (tokens[i].type != Semi_colon) i++;
      int expresion_size = i - expresion_beginning;
      new_tree[statements_num -1].statement_type = var_declaration_type;
      new_tree[statements_num -1].statement_value.var_declaration.var_name = var_name;
      new_tree[statements_num -1].statement_value.var_declaration.value = parse_expresion(&tokens[expresion_beginning], expresion_size);
      result_tree.statements_node = new_tree;
    }
    else {
      error("unkown statement type");
    }
  }
  result_tree.statements_count = statements_num;
  return result_tree;
}

#endif