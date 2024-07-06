#ifndef PARSER_H_
#define PARSER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"

// some near positive infinity number
#define BIG_NUM 999999


typedef struct Node_Type {
  Token token;
  enum {
    type_ptr_type,
    type_primitive_type,
  } type_type;
  union {
    struct Node_Type * type_ptr_value;
    Token type_primitive_value;
  } type_value;

} Node_Type;

typedef struct Node_Number {
  Token number_token;
} Node_Number;

typedef struct Node_Expresion {
  enum {
    expresion_number_type,
    expresion_identifier_type,
    expresion_binary_operation_type,
    expresion_unary_operation_type
  } expresion_type;
  union {
    Node_Number expresion_number_value;
    Token expresion_identifier_value;
    // binary and unary operations are a pointer so they can be recursive
    struct Node_Binary_Operation * expresion_binary_operation_value;
    struct Node_Unary_Operation * expresion_unary_operation_value;
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
    binary_operation_exp_type,
    binary_operation_big_type,
    binary_operation_les_type,
    binary_operation_equ_type
  } operation_type;
  Node_Expresion right_side;
} Node_Binary_Operation;

typedef struct Node_Unary_Operation {
  Node_Expresion expresion;
  enum {
    unary_operation_addr_type,
  } operation_type;
} Node_Unary_Operation;

typedef struct Node_Exit {
  Node_Expresion exit_code;
} Node_Exit;

typedef struct Node_Print {
  Node_Expresion chr;
} Node_Print;

typedef struct Node_Var_declaration {
  Token var_name;
  Node_Type type;
  Node_Expresion value;
} Node_Var_declaration;

typedef struct Node_Var_assignment {
  Token var_name;
  Node_Expresion value;
} Node_Var_assignment;


typedef struct Node_Scope {
  struct Node_Statement * statements_node;
  int statements_count;
} Node_Scope;

typedef struct Node_If {
  Node_Expresion condition;
  Node_Scope scope;
  bool has_else_block;
  Node_Scope else_block;
} Node_If;

typedef struct Node_While {
  Node_Expresion condition;
  Node_Scope scope;
} Node_While;

typedef struct Node_Statement {
  union {
    Node_Var_declaration var_declaration;
    Node_Exit exit_node;
    Node_Var_assignment var_assignment;
    Node_Scope scope;
    Node_If if_node;
    Node_Print print;
    Node_While while_node;
  } statement_value;
  enum {
    var_declaration_type,
    exit_node_type,
    var_assignment_type,
    scope_type,
    if_type,
    print_type,
    while_type
  } statement_type;
} Node_Statement;

typedef struct Node_Program {
  Node_Statement * statements_node;
  int statements_count;
} Node_Program;

// predeclare this functions to allow mutual recursion
Node_Program parser(const Token * tokens);
Node_Scope parse_scope(const Token * tokens, const int tokens_count);

// convert the string of a binary operation token into a enum that is a more manageable form
int get_binary_operation_type(Token operation) {
  int type;
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
  else if (compare_token_to_string(operation, ">")) {
    type = binary_operation_big_type;
  }
  else if (compare_token_to_string(operation, "<")) {
    type = binary_operation_les_type;
  }
  else if (compare_token_to_string(operation, "==")) {
    type = binary_operation_equ_type;
  }
  else {
    error("unkown binary operation in expresion");
  }
  return type;
}

// convert the string of a unary operation token into a enum that is a more manageable form
int get_unary_operation_type(Token operation) {
  int type;
  if (compare_token_to_string(operation, "&")) {
    type = unary_operation_addr_type;
  }
  else {
    error("unkown unary operation in expresion");
  }
  return type;
}

// get the precedence of a binary operator acording to the documentation
// the closest the value to 0 the less the precedence is
int get_binary_operation_precedence(Token operation) {
  const char * opers[] = {">", "==", "<", "+", "-", "%", "*", "/", "^"};
  const int precedes[] = { 0 ,  0 ,   0 ,  1 ,  1 ,  2 ,  2 ,  2 ,  3 };
  // find the idx of the matching string and return the corresponding precedence
  for (unsigned i = 0; i < sizeof(opers)/sizeof(*opers); i++) {
    if (compare_token_to_string(operation, opers[i])) {
      return precedes[i];
    }
  }
  error("unkown precedence of binary operation");
  // unreachable
  return -1;
}

// get the precedence of an unary operator acording to the documentation
// the closest the value to 0 the less the precedence is
int get_unary_operation_precedence(Token operation) {
  const char * opers[] = {"&"};
  const int precedes[] = { 4 };
  // find the idx of the matching string and return the corresponding precedence
  for (unsigned i = 0; i < sizeof(opers)/sizeof(*opers); i++) {
    if (compare_token_to_string(operation, opers[i])) {
      return precedes[i];
    }
  }
  error("unkown precedence of unary operation");
  // unreachable
  return -1;
}


// 'expr' must be the address of the first opening bracket in the expression
// returns the offset off the matching closing bracket,
//   if it couldnt find it prints an error an exits
int offset_of_match_bracket(const Token * expr, const int exprsz) {
  if (expr->type != Bracket || *expr->beginning != '(') {
    implementation_error("beginning of bracket expr is not open bracket");
  }

  int depth = 1;
  int offset = 1;
  while (depth != 0) {
    if (offset >= exprsz) {
      if (offset > 0) {
        error("expected a closing bracket");
      } else if (offset < 0) {
        error("expected a opening bracket");
      }
    }
    if (expr[offset].type == Bracket) {
      if (expr[offset].beginning[0] == '(') {
        depth++;
      }
      else if (expr[offset].beginning[0] == ')') {
        depth--;
      }
      else {
        implementation_error("symbol is of type Bracket but is not ( or )");
      }
    }
    offset++;
  }
  return offset -1;
}


// parses expresion recursively
Node_Expresion parse_expresion(const Token * expresion_beginning, const int size) {
  Node_Expresion result;
  if (size == 0) {
    error("expression must not be empty");
  }
  else if (size == 1) {
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
  else {
    if (expresion_beginning[0].type == Bracket && expresion_beginning[0].beginning[0] == '(' &&
        offset_of_match_bracket(expresion_beginning, size) == size -1) {
      result = parse_expresion(&expresion_beginning[1], size -2);
      return result;
    }
    Token operation;
    int min_oper_preced = BIG_NUM;
    // if the operation to parse is binary
    bool is_operation_bin = false;
    int left_side_beginning = 0;
    int left_side_size;
    int right_side_beginning;
    int right_side_size;
    // if the operation to parse is unary
    bool is_operation_uni = false;
    int uni_expresion_beginning;
    int uni_expresion_size;
    // indicates if the last token was an operation, it begins being true. used for distinguishing binary and unary operations
    bool was_last_op_or_null = true;
    // find the beginning and size of the left and right side of the expresion
    int i;
    for (i = 0; i < size; i++) {
      // if it finds a bracket skip it
      if (expresion_beginning[i].type == Bracket && expresion_beginning[i].beginning[0] == '(') {
        i += offset_of_match_bracket(&expresion_beginning[i], size);
      }
      // find the operation with the lowest precedence
      if (expresion_beginning[i].type == Operation) {
        if (was_last_op_or_null && !is_operation_bin) {
          is_operation_uni = true;
          min_oper_preced = get_unary_operation_precedence(expresion_beginning[i]);
          uni_expresion_beginning = i + 1;
          operation = expresion_beginning[i];
        }
        else if (!was_last_op_or_null && get_binary_operation_precedence(expresion_beginning[i]) < min_oper_preced) {
          is_operation_bin = true;
          is_operation_uni = false;
          min_oper_preced = get_binary_operation_precedence(expresion_beginning[i]);
          left_side_size = i;
          operation = expresion_beginning[i];
          right_side_beginning = i + 1;
        }
        was_last_op_or_null = true;
      }
      else {
        was_last_op_or_null = false;
      }
    }
    // if it did not found an operation report it
    if (min_oper_preced == BIG_NUM) {
      error("expected an operation in expresion");
    }
    if (is_operation_bin) {
      right_side_size = i - right_side_beginning;
      // create a new node and parse each side of the expresion
      Node_Binary_Operation * bin_operation = smalloc(sizeof(Node_Binary_Operation));
      // parse each side of the binary expresion recursively
      bin_operation->left_side = parse_expresion(&expresion_beginning[left_side_beginning], left_side_size);
      bin_operation->operation_type = get_binary_operation_type(operation);
      bin_operation->right_side = parse_expresion(&expresion_beginning[right_side_beginning], right_side_size);

      result.expresion_value.expresion_binary_operation_value = bin_operation;
      result.expresion_type = expresion_binary_operation_type;
    }
    else if (is_operation_uni) {
      uni_expresion_size = i - uni_expresion_beginning;
      // create a new node and parse the expresion
      Node_Unary_Operation * uni_operation = smalloc(sizeof(Node_Unary_Operation));
      // parse the unary expresion recursively
      uni_operation->expresion = parse_expresion(&expresion_beginning[uni_expresion_beginning], uni_expresion_size);
      uni_operation->operation_type = get_unary_operation_type(operation);

      result.expresion_value.expresion_unary_operation_value = uni_operation;
      result.expresion_type = expresion_unary_operation_type;
    }
    else {
      error("expected an operation in expresion while parsing it");
    }
  }
  return result;
}


// returns the offset of the next the semicolon token counting from the beginning pointer
// in case there is no semicolon or something happend, reports an error and exits
int next_semicolon_offset(const Token * beginning) {
  int offset;
  for (offset = 0; beginning[offset].type != Semi_colon; offset++) {
    Token token = beginning[offset];
    if (token.type == End_of_file) {
      error("could not find the expected semicolon");
    }
    if (token.type == Curly_bracket) {
      error("expected a semicolon");
    }
  }
  return offset;
}

// returns the offset of the next the '=' token counting from the beginning pointer
// if it could not find it, reports an error and exits
int next_asign_offset(const Token * beginning) {
  int offset;
  for (offset = 0; compare_token_to_string(beginning[offset], "=") == 0; offset++) {
    Token token = beginning[offset];
    if (token.type == End_of_file) {
      error("could not find the expected '='");
    }
    if (token.type == Semi_colon) {
      error("expected a '='");
    }
    if (token.type == Curly_bracket) {
      error("expected a '='");
    }
  }
  return offset;
}

// parses a type definition
Node_Type parse_type(const Token * type_beginning, const int type_sz) {
  if (type_sz == 0) {
    error("expected a type in varable declaration");
  }
  if (type_sz == 1) {
    if (!compare_token_to_string(type_beginning[0], "u64")) {
      error("expected a type");
    } 
    Node_Type type = {
      .token=type_beginning[0],
      .type_type=type_primitive_type,
      .type_value.type_primitive_value=type_beginning[0]
    };
    return type;
  }
  Node_Type type;
  int i = 0;
  if (compare_token_to_string(type_beginning[i], "ptr")) {
    type.token = type_beginning[i];
    type.type_type = type_ptr_type;
    type.type_value.type_ptr_value = smalloc(sizeof(*type.type_value.type_ptr_value));
    *type.type_value.type_ptr_value = parse_type(&type_beginning[i + 1], type_sz - 1); // add 1 to skip the already parsed "ptr", and sub 1 to account for that
    i += 1;
  }
  else {
    error("expected a type decorator");
  }
  return type;
}

// parse a scope the same way as a program
Node_Scope parse_scope(const Token * tokens, const int tokens_count) {
  Token * new_tokens = smalloc((tokens_count + 1) * sizeof(Token)); // add 1 for space of the EOF token
  // add EOF token so it can be parse tha same way as a program
  new_tokens[tokens_count] = (Token) {  .beginning = NULL,
                                        .length = 0,
                                        .type = End_of_file
                                     };
  memcpy(new_tokens, tokens, tokens_count * sizeof(Token));
  Node_Program temp_program = parser(new_tokens);
  free(new_tokens);
  Node_Scope scope;
  scope.statements_count = temp_program.statements_count;
  scope.statements_node = temp_program.statements_node;
  return scope;
}

// tries to parse the scope the ptr points to
// idx is a ptr to the index of the first '{' in the tokens
// idx will be updated to the matching '}'
Node_Scope parse_scope_at(const Token * tokens, int * idx) {
  int scope_count = 1; // counter of nested scopes to allow recursive scopes
  int i = *idx;
  // match the beginning of the scope with its ending accounting for recursive scopes
  while (scope_count != 0) {
    if (tokens[i].type == End_of_file) {
      error("unmatched open curly bracket");
    }
    i++;
    if (tokens[i].type == Curly_bracket) {
      if (compare_token_to_string(tokens[i], "{")) scope_count++;
      if (compare_token_to_string(tokens[i], "}")) scope_count--;
    }
  }
  Node_Scope scope = parse_scope(&tokens[*idx + 1], i - *idx -1); // add and substract 1 to avoid the original `{`, `}`
  *idx = i;
  return scope;
}

// parses exit statement
// tokens is the stream of tokens of the program
// index is indicates the 'exit' token in the tokens
// index will be updated to the corresponding ';'
Node_Exit parse_exit_at(const Token * tokens, int * idx) {
  int expresion_beginning = *idx +1; // add 1 to skip the "exit"
  *idx += next_semicolon_offset(&tokens[*idx]);
  int expresion_size = *idx - expresion_beginning;
  Node_Exit node_exit;
  node_exit.exit_code = parse_expresion(&tokens[expresion_beginning], expresion_size);
  return node_exit;
}

// parses print statement
// tokens is the stream of tokens of the program
// index is indicates the 'print' token in the tokens
// index will be updated to the corresponding ';'
Node_Print parse_print_at(const Token * tokens, int * idx) {
  int expresion_beginning = *idx +1; // add 1 to skip the "print"
  *idx += next_semicolon_offset(&tokens[*idx]);
  int expresion_size = *idx - expresion_beginning;
  Node_Print node_print;
  node_print.chr = parse_expresion(&tokens[expresion_beginning], expresion_size);
  return node_print;
}

// parses variable declaration statement
// tokens is the stream of tokens of the program
// index is indicates the variable name token in the tokens
// index will be updated to the corresponding ';'
Node_Var_declaration parse_var_declaration_at(const Token * tokens, int * idx) {
  if (tokens[*idx].type != Identifier) {
    error("expected an identifier in variable declaration");
  }
  Token var_name = tokens[*idx];

  *idx += 2; // add 2 to skip the variable name and the ':'
  const Token * type_beginning = &tokens[*idx];
  *idx += next_asign_offset(type_beginning);
  int type_sz = &tokens[*idx] - type_beginning;
  Node_Type type = parse_type(type_beginning, type_sz);

  int expresion_beginning = *idx +1; // add 1 to skip the '='
  *idx += next_semicolon_offset(&tokens[*idx]);
  int expresion_size = *idx - expresion_beginning;
  Node_Expresion expresion = parse_expresion(&tokens[expresion_beginning], expresion_size);

  Node_Var_declaration node_var_declaration = {
    .var_name = var_name,
    .type=type,
    .value=expresion
  };
  return node_var_declaration;
}

// parses variable assignment statement
// tokens is the stream of tokens of the program
// index is indicates the variable name token in the tokens
// index will be updated to the corresponding ';'
Node_Var_assignment parse_var_assignment_at(const Token * tokens, int * idx) {
  if (tokens[*idx].type != Identifier) {
    error("expected an identifier in variable assigment");
  }
  Token var_name = tokens[*idx];
  // add 2 to skip the var name and the "="
  int expresion_beginning = *idx + 2;
  *idx += next_semicolon_offset(&tokens[*idx]);
  int expresion_size = *idx - expresion_beginning;
  Node_Expresion expresion = parse_expresion(&tokens[expresion_beginning], expresion_size);

  Node_Var_assignment node_var_assigment = {
    .var_name=var_name,
    .value=expresion
  };
  return node_var_assigment;
}

// parses if (and else) statement
// tokens is the stream of tokens of the program
// index is indicates the 'if' token in the tokens
// index will be updated to the corresponding '}'
Node_If parse_if_at(const Token * tokens, int * idx) {
  // parse the condition
  *idx += 1; // add 1 to skip the 'if'
  const Token * expr = &tokens[*idx];
  int expr_sz = *idx;
  // FIX: improve this loop, it is very unsafe
  do { ++*idx; } while (tokens[*idx].type != Curly_bracket);
  expr_sz = *idx - expr_sz;
  Node_Expresion condition = parse_expresion(expr, expr_sz);

  // parse the if body
  Node_Scope scope = parse_scope_at(tokens, idx);
  Node_If node_if = (Node_If) {.condition=condition, scope=scope};

  if (compare_token_to_string(tokens[*idx + 1], "else")) {
    *idx += 2; // add 2 to skip the '}' and the 'else'
    node_if.has_else_block = true;
    node_if.else_block = parse_scope_at(tokens, idx);
  } else {
    node_if.has_else_block = false;
  }
  return node_if;
}

// parses while statement
// tokens is the stream of tokens of the program
// index is indicates the 'if' token in the tokens
// index will be updated to the corresponding '}'
Node_While parse_while_at(const Token * tokens, int * idx) {
  // parse the condition
  *idx += 1; // add 1 to skip the 'if'
  const Token * expr = &tokens[*idx];
  int expr_sz = *idx;
  // FIX: improve this loop, it is very unsafe
  do { ++*idx; } while (tokens[*idx].type != Curly_bracket);
  expr_sz = *idx - expr_sz;
  Node_Expresion condition = parse_expresion(expr, expr_sz);

  Node_Scope scope = parse_scope_at(tokens, idx);
  Node_While node_while = (Node_While) {
    .condition=condition,
    .scope=scope
  };
  return node_while;
}

// parses the tokens into a syntax tree
Node_Program parser(const Token * tokens) {
  Node_Program result_tree;
  int statements_num = 0;
  result_tree.statements_node = malloc(statements_num * sizeof(Node_Statement));
  result_tree.statements_count = 0;

  for (int i = 0; tokens[i].type != End_of_file; i++) {
    statements_num += 1;
    Node_Statement * new_tree = srealloc(result_tree.statements_node, statements_num * sizeof(Node_Statement));
    // exit node
    if (compare_token_to_string(tokens[i], "exit")) {
      Node_Statement stmt;
      stmt.statement_type = exit_node_type;
      stmt.statement_value.exit_node = parse_exit_at(tokens, &i);
      new_tree[statements_num -1] = stmt;
      result_tree.statements_node = new_tree;
    }
    else if (compare_token_to_string(tokens[i], "print")) {
      Node_Statement stmt;
      stmt.statement_type = print_type;
      stmt.statement_value.print = parse_print_at(tokens, &i);
      new_tree[statements_num -1] = stmt;
      result_tree.statements_node = new_tree;
    }
    else if (compare_token_to_string(tokens[i + 1], ":")) {
      Node_Statement stmt;
      stmt.statement_type = var_declaration_type;
      stmt.statement_value.var_declaration = parse_var_declaration_at(tokens, &i);
      new_tree[statements_num -1] = stmt;
      result_tree.statements_node = new_tree;
    }
    else if (compare_token_to_string(tokens[i + 1], "=")) {
      Node_Statement stmt;
      stmt.statement_type = var_assignment_type;
      stmt.statement_value.var_assignment = parse_var_assignment_at(tokens, &i);
      new_tree[statements_num -1] = stmt;
      result_tree.statements_node = new_tree;
    }
    else if (compare_token_to_string(tokens[i], "{")) {
      Node_Statement stmt;
      stmt.statement_type = scope_type;
      stmt.statement_value.scope = parse_scope_at(tokens, &i);;
      new_tree[statements_num -1] = stmt;
      result_tree.statements_node = new_tree;
    }
    else if (compare_token_to_string(tokens[i], "if")) {
      Node_Statement stmt;
      stmt.statement_type = if_type;
      stmt.statement_value.if_node = parse_if_at(tokens, &i);;
      new_tree[statements_num -1] = stmt;
      result_tree.statements_node = new_tree;
    }
    else if (compare_token_to_string(tokens[i], "while")) {
      Node_Statement stmt;
      stmt.statement_type = while_type;
      stmt.statement_value.while_node = parse_while_at(tokens, &i);;
      new_tree[statements_num -1] = stmt;
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