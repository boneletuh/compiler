#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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
  char * out = (char *) malloc(size + 1);
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
    const char simbol = string[i];
    switch (mode) {
      case searching_token:
        token_beginning = i;
        if (is_in_str(simbol, var_sym)) {
          mode = identifier;
          i -= 1;
        }

        else if (is_in_str(simbol, numb_sym)) {
          mode = number;
          i -= 1;
        }

        else if (is_in_str(simbol, oper_sym)) {
          mode = operation;
          i -= 1;
        }

        // deal here with sigle symbol tokens
        else if (simbol == ':') {
          mode = add_token;
          new_token.type = Colon;
        }

        else if (simbol == ';') {
          mode = add_token;
          new_token.type = Semi_colon;
        }
        // throw error if the symbol is not allowed
        else if (!is_in_str(simbol, separ_sym)) {
          // FIX: use error()
          // FIX: at the end of the file it detects the -1 symbol idk why
          if (simbol != -1) {
            printf("unkown type of symbol: %c  %hhu  %d\n", simbol, (unsigned char) string[i], i);
          }
        }
        
        break;
      
      case identifier:
        new_token.type = Identifier;
        if (!is_in_str(simbol, var_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case number:
        new_token.type = Number;
        if (!is_in_str(simbol, numb_sym)) {
          mode = add_token;
          i -= 1;
        }

        break;
      
      case operation:
        new_token.type = Operation;
        // the operation can be longer than 1 symbol
        if (!is_in_str(simbol, oper_sym)) {
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

        token_array[token_count] = new_token;

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
  union {
    Node_Var_declaration var_declaration;
    Node_Exit exit_node;
  } statement_value;
  enum {
    var_declaration_type,
    exit_node_type
  } statement_type;
} Node_Statement;

typedef struct Node_Programm {
  Node_Statement * statements_node;
  int statements_count;
} Node_Programm;

Node_Expresion parse_expresion(Token * expresion_beginning, int size) {
  if (size == 1) {
    Node_Expresion result;
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
    return result;
  }
  implementation_error("not implemented");
}

// parses the tokens into a syntax tree
Node_Programm parser(Token * tokens) {
  Node_Programm result_tree;
  int statements_num = 0;
  result_tree.statements_node = malloc(statements_num * sizeof(Node_Statement));
  result_tree.statements_count = 0;

  for (int i = 0; tokens[i].type != End_of_file; i++) {
    statements_num += 1;
    Node_Statement * new_tree = realloc(result_tree.statements_node, statements_num * sizeof(Node_Statement));
    if (new_tree == NULL) {
      error("can not allocate memory for new node while parsing");
    }
    // exit node
    if (compare_token_to_string(tokens[i], "exit")) {
      // get the number of tokens in the expresion, we add 1 to skip the "exit"
      int expresion_beginning = i +1;
      // TODO: improve this loop, it is very unsafe
      while (tokens[i].type != Semi_colon) { i++; };
      int expresion_size = i - expresion_beginning;
      new_tree[statements_num -1].statement_type = exit_node_type;
      new_tree[statements_num -1].statement_value.exit_node.exit_code = parse_expresion(&tokens[expresion_beginning], expresion_size);
      result_tree.statements_node = new_tree;
    }
    // FIX: this would crash if there is an identifier at the end of the file
    else if (compare_token_to_string(tokens[i + 1], "=")) {
      if (tokens[i].type != Identifier) {
        error("expected an identifier in variable assigment");
      }
      Token var_name = tokens[i];
      // get the number of tokens in the expresion, we add 2 to skip the var name and the "="
      int expresion_beginning = i +2;
      // TODO: improve this loop, it is very unsafe
      while (tokens[i].type != Semi_colon) { i++; };
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

void gen_expresion(Node_Expresion expresion, FILE * file_ptr) {
  if (expresion.expresion_type == expresion_number_type) {
    add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value.number_token);
  }
  else if (expresion.expresion_type == expresion_identifier_type) {
    add_token_to_file(file_ptr, expresion.expresion_value.expresion_identifier_value);
  }
  else {
    implementation_error("unexpected type in expresion");
  }
}

void gen_code(Node_Programm syntax_tree, char * out_file_name) {
  FILE * out_file_ptr = create_file(out_file_name);
  add_string_to_file(out_file_ptr, "#include <stdlib.h>\nvoid main() {\n");
  for (int i = 0; i < syntax_tree.statements_count; i++) {
    Node_Statement node = syntax_tree.statements_node[i];
    //var declaration node
    if (node.statement_type == var_declaration_type) {
      add_string_to_file(out_file_ptr, " int ");
      add_token_to_file(out_file_ptr, node.statement_value.var_declaration.var_name);
      add_string_to_file(out_file_ptr, " = ");
      gen_expresion(node.statement_value.var_declaration.value, out_file_ptr);
    }
    else if (node.statement_type == exit_node_type) {
      // exit node
      add_string_to_file(out_file_ptr, " exit(");
      gen_expresion(node.statement_value.exit_node.exit_code, out_file_ptr);
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


void compile(char * source_code_file, char * result_file) {
  // open the source cod file
  FILE * file = fopen(source_code_file, "r");
  char * code = file_contents(file);

  Token * tokens = lexer(code);
  //print_tokens(tokens);
  Node_Programm syntax_tree = parser(tokens);
  gen_code(syntax_tree, result_file);

  // free all the allocated memory
  free(code);
  printf("code freed\n");
  free(tokens);
  printf("tokens freed\n");
  free(syntax_tree.statements_node);
  printf("syntax tree freed\n");
}

int main(int argc, char ** argv) {
  if (argc < 3) {
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
