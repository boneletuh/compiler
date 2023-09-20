#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <stdarg.h>

// general error function
void error(char * string) {
  printf("Error: %s\n", string);
  exit(1);
}

// error that should only appeard while developing the compiler
// the user of the language sould not see this type of error
void implementation_error(char * string) {
  printf("Implementation Error: %s\n", string);
  exit(2);
}

typedef enum bool {
  false,
  true
} bool;


int file_size(FILE * file) {
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  return size;
}

char * file_contents(FILE * fptr) {
  if (fptr == NULL){
    error("can not open the code file");
  }

  int size = file_size(fptr);
  char * out = (char*) malloc(size + 1);
  if (out == NULL) {
    error("can not allocate memory for the program source code");
  }

  for (int i = 0; i < size; i++) {
    out[i] = getc(fptr);
  }
  out[size] = '\0';
  
  fclose(fptr); 
  return out;
}

FILE * create_file(char * file_name) {
  FILE * fptr = fopen(file_name,"w");
  if (fptr == NULL) {
    // FIX: use error()
    printf("could not create the file: %s", file_name);   
    exit(1);
  }
  return fptr; 
}

void add_string_to_file(FILE * file_ptr, char * string) {
  fprintf(file_ptr, string);
}


typedef struct Token {
  char * beginning;
  unsigned short length;
  enum {
    Identifier,
    Number,
    Operation,
    Colon,
    Semi_colon,
    End_of_file
  } type;
} Token;


void add_token_to_file(FILE * file_ptr, Token token) {
  fprintf(file_ptr, "%.*s", token.length, token.beginning);
}

bool is_in_str(char symbol, const char * string) {
  // TODO: add look up table for O(1) time
  for (int i = 0; string[i] != '\0'; i++) {
    if (string[i] == symbol) {
      return true;
    }
  }
  return false;
}


Token * lexer(char * string) {
  typedef enum {
    searching_token,
    identifier,
    number,
    operation,
    add_token
  } State;
  State mode = searching_token;

  const char * var_sym = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char * numb_sym = "0123456789";
  const char * oper_sym = "+-*/=%^";
  const char * separ_sym = " \r\t\n";

  int token_beginning = 0;
  int token_count = 0;
  Token * token_array = malloc(token_count * sizeof(Token));
  Token new_token;
  int i;
  for (i = 0; string[i] != '\0'; i++) {
    switch (mode) {
      case searching_token:
        token_beginning = i;
        if (is_in_str(string[i], var_sym)) {
          mode = identifier;
          i -= 1;
        }

        else if (is_in_str(string[i], numb_sym)) {
          mode = number;
          i -= 1;
        }

        else if (is_in_str(string[i], oper_sym)) {
          mode = operation;
          i -= 1;
        }

        // deal here with sigle symbol tokens
        else if (string[i] == ':') {
          mode = add_token;
          new_token.type = Colon;
        }

        else if (string[i] == ';') {
          mode = add_token;
          new_token.type = Semi_colon;
        }
        // throw error if the symbol is not allowed
        else if (!is_in_str(string[i], separ_sym)) {
          // FIX: use error()
          // FIX: at the end of the file it detects the -1 symbol idk why
          printf("unkown type of symbol: %c  %hu  %d\n", string[i], (unsigned int) string[i], i);
        }
        
        break;
      
      case identifier:
        new_token.type = Identifier;
        if (!is_in_str(string[i], var_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case number:
        new_token.type = Number;
        if (!is_in_str(string[i], numb_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case operation:
        new_token.type = Operation;
        // the operation can be longer than 1 symbol
        if (!is_in_str(string[i], oper_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case add_token:
        new_token.beginning = string + token_beginning;
        new_token.length = (unsigned short) (i - token_beginning);

        // add token to token_array
        Token * new_token_array = realloc(token_array, sizeof(Token) * (token_count + 1));
        if (new_token_array == NULL) { 
          error("can not allocate memory for new token");
        }

        token_array = new_token_array;

        token_array[token_count].beginning = new_token.beginning;
        token_array[token_count].length = new_token.length;
        token_array[token_count].type = new_token.type;
        //this is unnecessary just to clean the new_token
        new_token.beginning = NULL;
        new_token.length = 0;
        new_token.type = End_of_file;
        
        // print the token and update token beginning
        token_beginning = i;

        mode = searching_token;
        i -= 1;
        token_count += 1;
        break;

      default:
        // Fix: use implementation_error()
        printf("Error: unkown State with code: %d\n", mode);
        exit(1);
    }
  }
  // if theres still a token left add it to the token array
  if (mode != searching_token) {
    new_token.beginning = string + token_beginning;
    new_token.length = (unsigned short) (i - token_beginning);

    // add token to token_array
    Token * new_token_array = realloc(token_array, sizeof(Token) * (token_count + 1));
    if (new_token_array == NULL) { 
      error("can not allocate memory for last token");
    }

    token_array = new_token_array;

    token_array[token_count].beginning = new_token.beginning;
    token_array[token_count].length = new_token.length;
    token_array[token_count].type = new_token.type;
    
    token_count += 1;
  }
  // add EOF token to the end of token array
  Token * new_token_array = realloc(token_array, sizeof(Token) * (token_count + 1));
  if (new_token_array == NULL) { 
    error("can not allocate memory for EOF token");
  }

  token_array = new_token_array;

  token_array[token_count].beginning = NULL;
  token_array[token_count].length = 0;
  token_array[token_count].type = End_of_file;
  printf("number of tokens: %d\n", token_count);
  return token_array;
}

void print_tokens(Token * token_array) {
  for (int i = 0; token_array[i].type != End_of_file; i++) {
    printf("%.*s\n", token_array[i].length, token_array[i].beginning);
  }
}


typedef struct Statement {
  Token * statement_beginning;
  unsigned short tokens_amount;
} Statement;

Statement * separate_statements(Token * token_array) {
  int statement_count = 0;
  Statement * statement_array = malloc(statement_count * sizeof(Statement));
  Statement statement_buffer;
  statement_buffer.tokens_amount = 0;
  statement_buffer.statement_beginning = NULL;

  for (int i = 0; token_array[i].type != End_of_file; i++) {
    if (token_array[i].type == Semi_colon) {
      // add stament_buffer to statement_result
      Statement * new_statement_array = realloc(statement_array, sizeof(Statement) * (statement_count + 1));
      if (new_statement_array == NULL) { 
        error("can not allocate memory for new statement");
      }
      statement_array = new_statement_array;
      statement_array[statement_count] = statement_buffer;
      // clean stament_buffer
      statement_buffer.statement_beginning = NULL;
      statement_buffer.tokens_amount = 0;

      statement_count += 1;
    }
    else {
      // add the token to stament_buffer
      if (statement_buffer.tokens_amount == 0) {
        statement_buffer.statement_beginning = &(token_array[i]);
      }
      statement_buffer.tokens_amount += 1;
    }
  }

  statement_buffer.statement_beginning = NULL;
  statement_buffer.tokens_amount = 0;
  Statement * new_statement_array = realloc(statement_array, sizeof(Statement) * (statement_count + 1));
  if (new_statement_array == NULL) {
    error("can not allocate memory for last statement");
  }
  statement_array = new_statement_array;
  statement_array[statement_count] = statement_buffer;
  printf("number of statements: %d\n", statement_count);
  return statement_array;
}

void print_statements(Statement * statement_array) {
  Statement statement = statement_array[0];
  for (int i = 0; statement.statement_beginning != NULL; i++) {
    statement = statement_array[i];
    printf("| ");
    for (int j = 0; j < statement.tokens_amount; j++) {
      printf("%.*s\t", (statement.statement_beginning[j]).length, statement.statement_beginning[j].beginning);
    }
    putchar('\n');
  }
}


bool compare_token_to_string (Token token, char * string) {
  int i;
  for (i = 0; i < token.length && string[i] != '\0'; i++) {
    if (token.beginning[i] != string[i]) {
      return false;
    }
  }

  if (i == token.length && string[i] == '\0') {
    return true;
  }
  else {
    return false;
  }
}

typedef struct Node_Number {
  Token number_token;
} Node_Number;

typedef struct Node_Expresion {
  enum {
    expresion_number_type,
    expresion_identifier_type
  } expresion_type;
  union {
    Node_Number expresion_number_value;
    Token expresion_identifier_value;
  } expresion_value;
} Node_Expresion;

typedef struct Node_Exit {
  Node_Expresion exit_code;
} Node_Exit;

typedef struct Node_Var_declaration {
  Token var_name;
  Node_Expresion value;
} Node_Var_declaration;

typedef struct Node_Statement {
  Node_Var_declaration var_declaration;
} Node_Statement;

typedef struct Node_Programm {
  Node_Statement * statement_node;
  int statements_count;
  Node_Exit exit_node;
} Node_Programm;


Node_Programm parser(Statement * statements) {
  Node_Programm result_tree;
  int statement_num = 0;
  result_tree.statement_node = malloc(statement_num * sizeof(Node_Statement));
  Statement statement;
  
  for (int i = 0; statements[i].statement_beginning != NULL; i++) {
    statement = statements[i];
    if (statement.tokens_amount == 2) {
      // exit node
      if (compare_token_to_string(statement.statement_beginning[0], "exit")) {
        Token expresion = statement.statement_beginning[1];
        if (expresion.type == Number || expresion.type == Identifier) {
          if (expresion.type == Number) {
            result_tree.exit_node.exit_code.expresion_type = expresion_number_type;
            result_tree.exit_node.exit_code.expresion_value.expresion_number_value.number_token = expresion;
          }
          else if (expresion.type == Identifier) {
            result_tree.exit_node.exit_code.expresion_type = expresion_identifier_type;
            result_tree.exit_node.exit_code.expresion_value.expresion_identifier_value = expresion;
          }
          else {
            error("unkown expresion type in exit");
          }
        }
        else {
          error("not a valid type for exit");
        }
      }
      else {
        error("expected an exit");
      }
    }
    else if (statement.tokens_amount == 3) {
      // var declaration node
      if (statement.statement_beginning[0].type == Identifier) {
        if (compare_token_to_string(statement.statement_beginning[1], "=")) {
          Token var_name = statement.statement_beginning[0];
          Node_Expresion var_value;
          if (statement.statement_beginning[2].type == Number) {
            var_value.expresion_type = expresion_number_type;
            var_value.expresion_value.expresion_number_value.number_token = statement.statement_beginning[2];
          }
          else if (statement.statement_beginning[2].type == Identifier) {
            var_value.expresion_type = expresion_identifier_type;
            var_value.expresion_value.expresion_identifier_value = statement.statement_beginning[2];
          }
          else {
            error("unexpected type for variable declaration");
          }
          statement_num += 1;
          Node_Statement * new_statements = realloc(result_tree.statement_node, statement_num * sizeof(Node_Statement));
          if (new_statements == NULL) {
            error("can not allocate memory for new varible declaration branch");
          }
          new_statements[statement_num - 1].var_declaration.value = var_value;
          new_statements[statement_num - 1].var_declaration.var_name = var_name;
           result_tree.statement_node = new_statements;
        }
        else {
          error("expected a '=' in declaration");
        }
      }
      else {
        error("expected an identifier in declaration");
      }
    }
    else {
      error("unkown statement");
    }
  }
  result_tree.statements_count = statement_num;
  return result_tree;
}


void gen_code(Node_Programm syntax_tree, char * out_file_name) {
  FILE * out_file_ptr = create_file(out_file_name);
  add_string_to_file(out_file_ptr, "#include <stdlib.h>\nvoid main() {\n");

  //var declaration
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    add_string_to_file(out_file_ptr, " int ");
    add_token_to_file(out_file_ptr, syntax_tree.statement_node[i].var_declaration.var_name);
    add_string_to_file(out_file_ptr, " = ");
    if (syntax_tree.statement_node[i].var_declaration.value.expresion_type == expresion_number_type) {
      add_token_to_file(out_file_ptr, syntax_tree.statement_node[i].var_declaration.value.expresion_value.expresion_number_value.number_token);
    }
    else if (syntax_tree.statement_node[i].var_declaration.value.expresion_type == expresion_identifier_type) {
      add_token_to_file(out_file_ptr, syntax_tree.statement_node[i].var_declaration.value.expresion_value.expresion_identifier_value);
    }
    else {
      error("unexpected type in var declaration");
    }
    add_string_to_file(out_file_ptr, ";\n");
  }
  // exit
  add_string_to_file(out_file_ptr, " exit(");
  Node_Expresion _exit_code = syntax_tree.exit_node.exit_code;
  if (_exit_code.expresion_type == expresion_number_type) {
    add_token_to_file(out_file_ptr, _exit_code.expresion_value.expresion_number_value.number_token);
  }
  else if (_exit_code.expresion_type == expresion_identifier_type) {
    add_token_to_file(out_file_ptr, _exit_code.expresion_value.expresion_identifier_value);
  }
  else {
    error("can not recognize the type of expresion in code generation");
  }
  add_string_to_file(out_file_ptr, ");\n}");

  fclose(out_file_ptr);
}


void compile(char * source_code_file, char * result_file) {
  FILE * file = fopen(source_code_file, "r");
  char * code = file_contents(file);

  Token * tokens = lexer(code);
  print_tokens(tokens);
  Statement * statements = separate_statements(tokens);
  print_statements(statements);
  Node_Programm syntax_tree = parser(statements);
  gen_code(syntax_tree, result_file);

  // free all the allocated memory
  free(code);
  printf("code freed\n");
  free(tokens);
  printf("tokens freed\n");
  free(statements);
  printf("statements freed\n");
  // FIX: free the memory of the branches of the tree
}

int main(int argc, char * argv[]) {
  for (int i = 0; i < argc; i++) {
    printf("%d: %s\n", i, argv[i]);
  }

  //start clock
  clock_t start, end;
  double time;
  start = clock();

  char * code = "code.txt";
  char * out_file = "out.c";

  compile(code, out_file);

  // time it
  end = clock();
  time = ((float) (end - start)) / CLOCKS_PER_SEC;
  printf("\n#######\ntime: %lf\n", time);

  printf("code finnished succesfully\n");

  return 0;
}
