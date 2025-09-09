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
	PTR_sz = 8  // TODO: only in 64 bit platforms
};

// returns the size of the given type in bytes
static int get_size_of_type(const Node_Type type) {
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

		case type_func_type:
			return PTR_sz;
			break;
	}
	implementation_error("tried to get the size of an unkown type");
	// unreachable
	return -1;
}


/* * * * * * * * * * *
 * Generating C code *
 * * * * * * * * * * */

// type containning the variables of the program and their types
typedef struct C_Variables_List {
	int variables_count;
	Node_Type * var_types_list;
	Token * var_tokens_list;
} C_Variables_List;

typedef struct C_Scopes_List {
	int scopes_count;
	C_Variables_List * variables;
} C_Scopes_List;


// add a variable to the last scope of list of variables, and its place on the stack
static void C_append_var_to_var_list(C_Scopes_List * scopes, const Token variable, const Node_Type type) {
	C_Variables_List * last_scope = &scopes->variables[scopes->scopes_count -1];
	last_scope->variables_count++;
	last_scope->var_types_list = srealloc(last_scope->var_types_list, last_scope->variables_count * sizeof(*last_scope->var_types_list));
	last_scope->var_types_list[last_scope->variables_count -1] = type;
	last_scope->var_tokens_list = srealloc(last_scope->var_tokens_list, last_scope->variables_count * sizeof(*last_scope->var_tokens_list));
	last_scope->var_tokens_list[last_scope->variables_count -1] = variable;
}

// create a new scope and append it to the end of the list of scopes
static void C_create_scope(C_Scopes_List * scopes) {
	scopes->scopes_count++;
	scopes->variables = srealloc(scopes->variables, scopes->scopes_count * sizeof(*scopes->variables));
	scopes->variables[scopes->scopes_count -1].variables_count = 0;
	scopes->variables[scopes->scopes_count -1].var_types_list = malloc(scopes->variables[scopes->scopes_count -1].variables_count * sizeof(int));
	scopes->variables[scopes->scopes_count -1].var_tokens_list = malloc(scopes->variables[scopes->scopes_count -1].variables_count * sizeof(Token));
}

// remove the last scope from the list of scopes
static void C_remove_scope(C_Scopes_List * scopes) {
	scopes->scopes_count--;
	scopes->variables = srealloc(scopes->variables, scopes->scopes_count * sizeof(*scopes->variables));
}

static void C_free_scopes_list(const C_Scopes_List scopes) {
	for (int i = 0; i < scopes.scopes_count; i++) {
		free(scopes.variables[i].var_types_list);
		free(scopes.variables[i].var_tokens_list);
	}
	free(scopes.variables);
}

Node_Type C_get_type_of_variable(const Token variable, const C_Scopes_List scopes) {
	for (int i = 0; i < scopes.scopes_count; i++) {
		for (int j = 0; j < scopes.variables[i].variables_count; j++) {
			if (compare_str_of_tokens(scopes.variables[i].var_tokens_list[j], variable)) {
				return scopes.variables[i].var_types_list[j];
			}
		}
	}
	implementation_error("could not find variable in scopes list in C code generation");
	return (Node_Type){};
}

Node_Type C_get_type_of_expresion(const Node_Expresion expresion, const C_Scopes_List scopes) {
	switch (expresion.expresion_type) {
		case expresion_number_type: {
			Node_Type type;
			type.token.beginning = smalloc(4); // FIX: this also leaks memory
			strcpy(type.token.beginning, "u64");
			type.token.length = 3;
			type.type_type = type_primitive_type;
			type.type_value.type_primitive_value = type.token;
			return type;
		}

		case expresion_identifier_type: {
			return C_get_type_of_variable(expresion.expresion_value.expresion_identifier_value, scopes);
		}

		case expresion_binary_operation_type: {
			// does not matter if its `left_side` or `right_side`
			Node_Expresion lhs_expr = expresion.expresion_value.expresion_binary_operation_value->left_side;
			return  C_get_type_of_expresion(lhs_expr, scopes);
		}

		case expresion_unary_operation_type: {
			Node_Unary_Operation uni_operation = *expresion.expresion_value.expresion_unary_operation_value;
			Node_Expresion uni_expresion = uni_operation.expresion;
			Node_Type type;
			// if the operation is the address operator `&`, the returned type is a pointer to the type of the expression
			if (uni_operation.operation_type == unary_operation_addr_type) {
				type.token = NULL_TOKEN;
				type.type_type = type_ptr_type;
				type.type_value.type_ptr_value = smalloc(sizeof(*type.type_value.type_ptr_value)); // FIX: this leaks memory
				*type.type_value.type_ptr_value = C_get_type_of_expresion(uni_expresion, scopes);
			}
			// if the operation is the dereference operator `*`, the returned type is type the pointer holds
			else if (uni_operation.operation_type == unary_operation_deref_type) {
				type = *C_get_type_of_expresion(uni_expresion, scopes).type_value.type_ptr_value;
			}
			else {
				type = C_get_type_of_expresion(uni_expresion, scopes);
			}
			return type;
		}

		case expresion_array_type: {
			Node_Type type;
			type.token = NULL_TOKEN;
			type.type_type = type_array_type;
			type.type_value.type_array_value = smalloc(sizeof(*type.type_value.type_array_value));
			type.type_value.type_array_value->primitive_type = smalloc(sizeof(*type.type_value.type_array_value->primitive_type));
			*type.type_value.type_array_value->primitive_type = C_get_type_of_expresion(expresion.expresion_value.expresion_array_value->elements[0], scopes);
			// FIX: convert the number to token in a more reasonable way
			type.type_value.type_array_value->elements_count.beginning = smalloc(2);
			type.type_value.type_array_value->elements_count.length = 2;
			type.type_value.type_array_value->elements_count.beginning[0] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 10;
			type.type_value.type_array_value->elements_count.beginning[1] = '0' + expresion.expresion_value.expresion_array_value->elements_count % 10;
			type.type_value.type_array_value->elements_count.type = Number;
			return type;
		}

		case expresion_func_call_type: {
			Token func_name = expresion.expresion_value.expresion_func_call_value->func_name;
			// FIX: if the expresion returns nothing return an 'empty' type
			Node_Type return_type = C_get_type_of_variable(func_name, scopes).type_value.type_func_value->return_type;
			return return_type;
		}
	}
	return (Node_Type) {};
}

static bool C_now_compiling_a_declaration_assignment = false;

static void gen_C_scope(const Node_Scope scope, FILE * out_file_name, C_Scopes_List *);
void gen_C_code(const Node_Program syntax_tree, const char * out_file_name);

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
		
		case type_func_type:
			Node_Func_type func_type = *type.type_value.type_func_value;
			if (func_type.returns_value) {
				gen_C_type(out_file_ptr, func_type.return_type);
			} else {
				add_string_to_file(out_file_ptr, "void ");
			}

			add_string_to_file(out_file_ptr, "(");
			for (int i = 0; i < func_type.args_count; i++) {
				if (i != 0) {
					add_string_to_file(out_file_ptr, ", ");
				}
				gen_C_type(out_file_ptr, func_type.args_type[i]);
			}
			add_string_to_file(out_file_ptr, ") ");

			break;
	}
}

static void gen_C_expresion(const Node_Expresion expresion, FILE * file_ptr, const C_Scopes_List scopes) {
	switch (expresion.expresion_type) {
		case expresion_number_type:
			add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value);
			break;

		case expresion_identifier_type:
			add_token_to_file(file_ptr, expresion.expresion_value.expresion_identifier_value);
			break;

		case expresion_binary_operation_type:
			add_string_to_file(file_ptr, "(");
			gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side, file_ptr, scopes);
			switch (expresion.expresion_value.expresion_binary_operation_value->operation_type) {
				case binary_operation_sum_type:
					add_string_to_file(file_ptr, " + ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_sub_type:
					add_string_to_file(file_ptr, " - ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_mul_type:
					add_string_to_file(file_ptr, " * ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_div_type:
					add_string_to_file(file_ptr, " / ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_mod_type:
					add_string_to_file(file_ptr, " % ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_big_type:
					add_string_to_file(file_ptr, " > ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_les_type:
					add_string_to_file(file_ptr, " < ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_equ_type:
					add_string_to_file(file_ptr, " == ");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					break;

				case binary_operation_access_type:
					add_string_to_file(file_ptr, "[");
					gen_C_expresion(expresion.expresion_value.expresion_binary_operation_value->right_side, file_ptr, scopes);
					add_string_to_file(file_ptr, "]");
					break;
			}
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
			}
			add_string_to_file(file_ptr, "(");
			gen_C_expresion(expresion.expresion_value.expresion_unary_operation_value->expresion, file_ptr, scopes);
			add_string_to_file(file_ptr, ")");
			break;

		case expresion_array_type:
			// this exists because stupid C rules
			if (!C_now_compiling_a_declaration_assignment) {
				add_string_to_file(file_ptr, "(");
				gen_C_type(file_ptr, C_get_type_of_expresion(expresion, scopes));
				add_string_to_file(file_ptr, ")");
			}
			add_string_to_file(file_ptr, "{");
			// generate each element with a preceding comma execept for the first one
			if (expresion.expresion_value.expresion_array_value->elements_count >= 1) {
				gen_C_expresion(expresion.expresion_value.expresion_array_value->elements[0], file_ptr, scopes);
			}
			for (int i = 1; i < expresion.expresion_value.expresion_array_value->elements_count; i++) {
				add_string_to_file(file_ptr, ", ");
				gen_C_expresion(expresion.expresion_value.expresion_array_value->elements[i], file_ptr, scopes);
			}
			add_string_to_file(file_ptr, "}");
			break;
		
		case expresion_func_call_type:
			add_string_to_file(file_ptr, "(");
			add_token_to_file(file_ptr, expresion.expresion_value.expresion_func_call_value->func_name);
			add_string_to_file(file_ptr, "(");
			for (int i = 0; i < expresion.expresion_value.expresion_func_call_value->args_count; i++) {
				if (i != 0) {
					add_string_to_file(file_ptr, ", ");
				}
				add_string_to_file(file_ptr, "(");
				gen_C_expresion(expresion.expresion_value.expresion_func_call_value->args[i], file_ptr, scopes);
				add_string_to_file(file_ptr, ")");
			}
			add_string_to_file(file_ptr, "))");
			break;
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
		
		case type_func_type:
			Node_Func_type func_type = *type.type_value.type_func_value;

			if (func_type.returns_value) {
				gen_C_type(out_file_ptr, func_type.return_type);
			} else {
				add_string_to_file(out_file_ptr, "void ");
			}

			add_string_to_file(out_file_ptr, "(*");
			add_token_to_file(out_file_ptr, var_name);
			add_string_to_file(out_file_ptr, ")");
			has_var_name_been_written = true;

			add_string_to_file(out_file_ptr, "(");
			for (int i = 0; i < func_type.args_count; i++) {
				if (i != 0) {
					add_string_to_file(out_file_ptr, ", ");
				}
				gen_C_type(out_file_ptr, func_type.args_type[i]);
			}
			add_string_to_file(out_file_ptr, ") ");
			
			break;
	}
	if (!has_var_name_been_written) {
		add_token_to_file(out_file_ptr, var_name);
	}
}

static void gen_C_statement(const Node_Statement stmt, FILE * out_file_ptr, C_Scopes_List * scopes) {
	switch (stmt.statement_type) {
		case var_declaration_type:
			C_now_compiling_a_declaration_assignment = true;
			add_string_to_file(out_file_ptr, " ");
			gen_C_var_decl_type_and_name(out_file_ptr, stmt.statement_value.var_declaration.var_name, stmt.statement_value.var_declaration.type);

			add_string_to_file(out_file_ptr, " = ");
			gen_C_expresion(stmt.statement_value.var_declaration.value, out_file_ptr, *scopes);

			C_append_var_to_var_list(scopes, stmt.statement_value.var_declaration.var_name, stmt.statement_value.var_declaration.type);
			C_now_compiling_a_declaration_assignment = false;
			break;

		case exit_node_type:
			add_string_to_file(out_file_ptr, " exit((uint64_t)");
			gen_C_expresion(stmt.statement_value.exit_node.exit_code, out_file_ptr, *scopes);
			add_string_to_file(out_file_ptr, ")");
			break;

		case print_type:
			add_string_to_file(out_file_ptr, " putchar(");
			gen_C_expresion(stmt.statement_value.print.chr, out_file_ptr, *scopes);
			add_string_to_file(out_file_ptr, "&0xff)");
			break;

		case var_assignment_type:
			Node_Var_assignment var_assgn = stmt.statement_value.var_assignment;
			switch (var_assgn.destination.destination_type) {
				case assgn_dest_var_name_type:
					add_string_to_file(out_file_ptr, " ");
					add_token_to_file(out_file_ptr, var_assgn.destination.destination_value.var_name);
					add_string_to_file(out_file_ptr, " = ");
					gen_C_expresion(var_assgn.value, out_file_ptr, *scopes);
					break;

				case assgn_dest_deref_type:
					add_string_to_file(out_file_ptr, " *");
					gen_C_expresion(*var_assgn.destination.destination_value.deref_expresion, out_file_ptr, *scopes);;
					add_string_to_file(out_file_ptr, " = ");
					gen_C_expresion(var_assgn.value, out_file_ptr, *scopes);
					break;
				
				case assgn_dest_subscript_type:
					add_string_to_file(out_file_ptr, " ");
					add_token_to_file(out_file_ptr, var_assgn.destination.destination_value.array_name);
					add_string_to_file(out_file_ptr, "[");
					gen_C_expresion(*var_assgn.destination.destination_value.index_expr, out_file_ptr, *scopes);
					add_string_to_file(out_file_ptr, "] = ");
					gen_C_expresion(var_assgn.value, out_file_ptr, *scopes);
					break;
			}
			break;

		case scope_type:
			add_string_to_file(out_file_ptr, " {\n");
			gen_C_scope(stmt.statement_value.scope, out_file_ptr, scopes);
			add_string_to_file(out_file_ptr, " }");
			break;
		
		case if_type:
			// generate the condition
			add_string_to_file(out_file_ptr, " if ( ");
			gen_C_expresion(stmt.statement_value.if_node.condition, out_file_ptr, *scopes);
			// generate the scope
			add_string_to_file(out_file_ptr, " ) {\n");
			gen_C_scope(stmt.statement_value.if_node.scope, out_file_ptr, scopes);
			add_string_to_file(out_file_ptr, " }");
			// generate the else block
			if (stmt.statement_value.if_node.has_else_block) {
				add_string_to_file(out_file_ptr, " else {\n");
				gen_C_scope(stmt.statement_value.if_node.else_block, out_file_ptr, scopes);
				add_string_to_file(out_file_ptr, " }");
			}
			break;

		case while_type:
			// generate the condition
			add_string_to_file(out_file_ptr, " while ( ");
			gen_C_expresion(stmt.statement_value.while_node.condition, out_file_ptr, *scopes);
			// generate the scope
			add_string_to_file(out_file_ptr, " ) {\n");
			gen_C_scope(stmt.statement_value.while_node.scope, out_file_ptr, scopes);
			add_string_to_file(out_file_ptr, " }");
			break;
		
		case func_def_type:
			// the function have already been generated before
			return;
		
		case return_type:
			add_string_to_file(out_file_ptr, " return ");
			if (stmt.statement_value.return_node.returns_value) {
				gen_C_expresion(stmt.statement_value.return_node.ret_value, out_file_ptr, *scopes);
			}
			break;
		
		case expresion_stmt_type:
			gen_C_expresion(stmt.statement_value.expresion_stmt, out_file_ptr, *scopes);
			break;
	}
	add_string_to_file(out_file_ptr, ";\n");
}

static void gen_C_scope(const Node_Scope scope, FILE * out_file_ptr, C_Scopes_List * scopes) {
	C_create_scope(scopes);
	for (int i = 0; i < scope.statements_count; i++) {
		gen_C_statement(scope.statements_node[i], out_file_ptr, scopes);
	}
	C_remove_scope(scopes);
}

static void gen_C_function(const Node_Func_def func, FILE * out_file_ptr, C_Scopes_List * scopes) {
	// int f()
	// int (*f())(int, int)
	// returned_func_return_type (*func_name(args...))(returned_func_args)
	// FIX: this does not work for arbitrary depth recursive function returns of functions
	Node_Func_type func_type = *func.type.type_value.type_func_value;

	if (func_type.returns_value) {
		if (func_type.return_type.type_type == type_func_type) {
			if (func_type.return_type.type_value.type_func_value->returns_value) {
				gen_C_type(out_file_ptr, func_type.return_type.type_value.type_func_value->return_type);
			} else {
				add_string_to_file(out_file_ptr, "void ");
			}
		} else {
			gen_C_type(out_file_ptr, func_type.return_type);
		}
	} else {
		add_string_to_file(out_file_ptr, "void ");
	}

	if (func_type.returns_value && func_type.return_type.type_type == type_func_type) {
		add_string_to_file(out_file_ptr, "(*");
	}
	add_token_to_file(out_file_ptr, func.name);
	add_string_to_file(out_file_ptr, "(");
	for (int i = 0; i < func_type.args_count; i++) {
		if (i != 0) {
			add_string_to_file(out_file_ptr, ", ");
		}
		gen_C_var_decl_type_and_name(out_file_ptr, func_type.args_name[i], func_type.args_type[i]);
	}
	add_string_to_file(out_file_ptr, ") ");
	if (func_type.returns_value && func_type.return_type.type_type == type_func_type) {
		add_string_to_file(out_file_ptr, ")");
	}

	if (func_type.returns_value && func_type.return_type.type_type == type_func_type) {
		add_string_to_file(out_file_ptr, "(");
		for (int i = 0; i < func_type.return_type.type_value.type_func_value->args_count; i++) {
			if (i != 0) {
				add_string_to_file(out_file_ptr, ", ");
			}
			gen_C_type(out_file_ptr, func_type.return_type.type_value.type_func_value->args_type[i]);
		}
		add_string_to_file(out_file_ptr, ") ");
	}
	for (int i = 0; i < func.type.type_value.type_func_value->args_count; i++) {
		C_append_var_to_var_list(scopes, func.type.type_value.type_func_value->args_name[i], func.type.type_value.type_func_value->args_type[i]);
	}

	add_string_to_file(out_file_ptr, "{\n");
	gen_C_scope(func.scope, out_file_ptr, scopes);
	add_string_to_file(out_file_ptr, "}\n\n");
}

// it generates C code
void gen_C_code(const Node_Program syntax_tree, const char * out_file_name) {
	FILE * out_file_ptr = create_file(out_file_name);

	// this will hold all the variables from all the scopes
	C_Scopes_List scopes;
	scopes.scopes_count = 0;
	scopes.variables = malloc(scopes.scopes_count * sizeof(C_Scopes_List));
	C_create_scope(&scopes); // create first global scope

	add_string_to_file(out_file_ptr, "#include <stdlib.h>\n#include <stdio.h>\n#include <stdint.h>\n");
	for (int i = 0; i < syntax_tree.statements_count; i++) {
		if (syntax_tree.statements_node[i].statement_type == func_def_type) {
			Node_Func_def func = syntax_tree.statements_node[i].statement_value.func_def;
			gen_C_function(func, out_file_ptr, &scopes);
			C_append_var_to_var_list(&scopes, func.name, func.type);
		}
	}

	add_string_to_file(out_file_ptr, "int main() {\n");
	for (int i = 0; i < syntax_tree.statements_count; i++) {
		Node_Statement node = syntax_tree.statements_node[i];
		gen_C_statement(node, out_file_ptr, &scopes);
	}
	add_string_to_file(out_file_ptr, "}\n");

	C_free_scopes_list(scopes);

	fclose(out_file_ptr);
}



/* * * * * * * * * * * *
 * Generating ASM code *
 * * * * * * * * * * * */

typedef struct ASM_Memory_place {
	enum {
		stack_place_type,
		arg_place_type,
		string_place_type
	} place_type;
	union {
		int stack_place_value;
		int arg_place_value;
		char * string_place_value;
	} place_value;
} ASM_Memory_place;

// type containning the variables of the program and their places in memory
typedef struct ASM_Variables_List {
	// FIX: rename properly this variables
	int var_stack_size;
	ASM_Memory_place * var_mem_places_list;
	Token * var_stack_tokens_list;
	Node_Type * var_stack_types_list;
} ASM_Variables_List;

typedef struct ASM_Scopes_List {
	int scopes_count;
	ASM_Variables_List * variables;
} ASM_Scopes_List;

// find the place in where the variable is located in the stack
static ASM_Memory_place find_var_mem_place(const ASM_Scopes_List vars_list, const Token var) {
	for (int i = 0; i < vars_list.scopes_count; i++) {
		for (int j = 0; j < vars_list.variables[i].var_stack_size; j++) {
			if (compare_str_of_tokens(var, vars_list.variables[i].var_stack_tokens_list[j])) {
				return vars_list.variables[i].var_mem_places_list[j];
			}
		}
	}
	// couldnt find the variable in the list
	// this sould have been detected by the checker
	implementation_error("could not find variable in variable list");
	// unreachable
	return (ASM_Memory_place){};
}

// check if there has benn declared a variables with a certain name
static bool check_if_var_declared(const ASM_Scopes_List vars_list, const char * var) {
	for (int i = 0; i < vars_list.scopes_count; i++) {
		for (int j = 0; j < vars_list.variables[i].var_stack_size; j++) {
			if (compare_token_to_string(vars_list.variables[i].var_stack_tokens_list[j], var)) {
				return true;
			}
		}
	}
	return false;
}

// add a variable to the last scope of list of variables, and its place on the stack
static void NASM_append_var_to_var_list(ASM_Scopes_List * scopes, const Token variable, const ASM_Memory_place mem_place, const Node_Type type) {
	ASM_Variables_List * last_scope = &scopes->variables[scopes->scopes_count -1];
	last_scope->var_stack_size++;

	last_scope->var_mem_places_list = srealloc(last_scope->var_mem_places_list, last_scope->var_stack_size * sizeof(*last_scope->var_mem_places_list));
	last_scope->var_mem_places_list[last_scope->var_stack_size -1] = mem_place;

	last_scope->var_stack_tokens_list = srealloc(last_scope->var_stack_tokens_list, last_scope->var_stack_size * sizeof(*last_scope->var_stack_tokens_list));
	last_scope->var_stack_tokens_list[last_scope->var_stack_size -1] = variable;

	last_scope->var_stack_types_list = srealloc(last_scope->var_stack_types_list, last_scope->var_stack_size * sizeof(*last_scope->var_stack_types_list));
	last_scope->var_stack_types_list[last_scope->var_stack_size -1] = type;
}

// create a new scope and append it to the end of the list of scopes
static void NASM_create_scope(ASM_Scopes_List * scopes) {
	scopes->scopes_count++;
	scopes->variables = srealloc(scopes->variables, scopes->scopes_count * sizeof(*scopes->variables));
	scopes->variables[scopes->scopes_count -1].var_stack_size = 0;
	scopes->variables[scopes->scopes_count -1].var_stack_tokens_list = malloc(scopes->variables[scopes->scopes_count -1].var_stack_size * sizeof(Token));
	scopes->variables[scopes->scopes_count -1].var_mem_places_list = malloc(scopes->variables[scopes->scopes_count -1].var_stack_size * sizeof(ASM_Memory_place));
	scopes->variables[scopes->scopes_count -1].var_stack_types_list = malloc(scopes->variables[scopes->scopes_count -1].var_stack_size * sizeof(Node_Type));
}

// remove the last scope from the list of scopes
static void NASM_remove_scope(ASM_Scopes_List * scopes) {
	scopes->scopes_count--;
	scopes->variables = srealloc(scopes->variables, scopes->scopes_count * sizeof(*scopes->variables));
}

static void NASM_free_scopes_list(ASM_Scopes_List scopes) {
	for (int i = 0; i < scopes.scopes_count; i++) {
		for (int j = 0; j < scopes.variables[i].var_stack_size; j++) {
			if (scopes.variables[i].var_mem_places_list[j].place_type == string_place_type) {
				free(scopes.variables[i].var_mem_places_list[j].place_value.string_place_value);
			}
		}
		free(scopes.variables[i].var_mem_places_list);
		free(scopes.variables[i].var_stack_tokens_list);
		free(scopes.variables[i].var_stack_types_list);
	}
	free(scopes.variables);
}

Node_Type NASM_get_type_of_variable(const Token variable, const ASM_Scopes_List scopes) {
	for (int i = 0; i < scopes.scopes_count; i++) {
		for (int j = 0; j < scopes.variables[i].var_stack_size; j++) {
			if (compare_str_of_tokens(scopes.variables[i].var_stack_tokens_list[j], variable)) {
				return scopes.variables[i].var_stack_types_list[j];
			}
		}
	}
	D_print_token(variable);
	implementation_error("could not find variable in scopes list in ASM code generation");
	return (Node_Type){};
}

Node_Type NASM_get_type_of_expresion(const Node_Expresion expresion, const ASM_Scopes_List scopes) {
	switch (expresion.expresion_type) {
		case expresion_number_type: {
			Node_Type type;
			type.token.beginning = smalloc(4); // FIX: this also leaks memory
			strcpy(type.token.beginning, "u64");
			type.token.length = 3;
			type.type_type = type_primitive_type;
			type.type_value.type_primitive_value = type.token;
			return type;
		}

		case expresion_identifier_type: {
			return NASM_get_type_of_variable(expresion.expresion_value.expresion_identifier_value, scopes);
		}

		case expresion_binary_operation_type: {
			// does not matter if its `left_side` or `right_side`
			Node_Expresion lhs_expr = expresion.expresion_value.expresion_binary_operation_value->left_side;
			return NASM_get_type_of_expresion(lhs_expr, scopes);
		}

		case expresion_unary_operation_type: {
			Node_Unary_Operation uni_operation = *expresion.expresion_value.expresion_unary_operation_value;
			Node_Expresion uni_expresion = uni_operation.expresion;
			Node_Type type;
			// if the operation is the address operator `&`, the returned type is a pointer to the type of the expression
			if (uni_operation.operation_type == unary_operation_addr_type) {
				type.token = NULL_TOKEN;
				type.type_type = type_ptr_type;
				type.type_value.type_ptr_value = smalloc(sizeof(*type.type_value.type_ptr_value)); // FIX: this leaks memory
				*type.type_value.type_ptr_value = NASM_get_type_of_expresion(uni_expresion, scopes);
			}
			// if the operation is the dereference operator `*`, the returned type is type the pointer holds
			else if (uni_operation.operation_type == unary_operation_deref_type) {
				type = *NASM_get_type_of_expresion(uni_expresion, scopes).type_value.type_ptr_value;
			}
			else {
				type = NASM_get_type_of_expresion(uni_expresion, scopes);
			}
			return type;
		}

		case expresion_array_type: {
			Node_Type type;
			type.token = NULL_TOKEN;
			type.type_type = type_array_type;
			type.type_value.type_array_value = smalloc(sizeof(*type.type_value.type_array_value));
			type.type_value.type_array_value->primitive_type = smalloc(sizeof(*type.type_value.type_array_value->primitive_type));
			*type.type_value.type_array_value->primitive_type = NASM_get_type_of_expresion(expresion.expresion_value.expresion_array_value->elements[0], scopes);
			// FIX: convert the number to token in a more reasonable way
			type.type_value.type_array_value->elements_count.beginning = smalloc(2);
			type.type_value.type_array_value->elements_count.length = 2;
			type.type_value.type_array_value->elements_count.beginning[0] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 10;
			type.type_value.type_array_value->elements_count.beginning[1] = '0' + expresion.expresion_value.expresion_array_value->elements_count % 10;
			type.type_value.type_array_value->elements_count.type = Number;
			return type;
		}

		case expresion_func_call_type: {
			Node_Type func_type = NASM_get_type_of_variable(expresion.expresion_value.expresion_func_call_value->func_name, scopes);
			Node_Type func_return_type = func_type.type_value.type_func_value->return_type;
			return func_return_type;
		}
	}
	return (Node_Type) {};
}

// returns the amount of memory that an expresion uses in the stack
static int get_stack_size_of_expr(const Node_Expresion expr, const ASM_Scopes_List scopes) {
	int bytes_count = 0;
	switch (expr.expresion_type) {
		case expresion_number_type:
			bytes_count = U64_sz;
			break;

		case expresion_identifier_type:
			bytes_count = get_size_of_type(NASM_get_type_of_variable(expr.expresion_value.expresion_identifier_value, scopes));
			break;

		case expresion_binary_operation_type:
			bytes_count = get_stack_size_of_expr(expr.expresion_value.expresion_binary_operation_value->left_side, scopes) + get_stack_size_of_expr(expr.expresion_value.expresion_binary_operation_value->right_side, scopes);
			break;

		case expresion_unary_operation_type:
			bytes_count = get_stack_size_of_expr(expr.expresion_value.expresion_unary_operation_value->expresion, scopes);
			break;

		case expresion_array_type:
			for (int i = 0; i < expr.expresion_value.expresion_array_value->elements_count; i++) {
				bytes_count += get_stack_size_of_expr(expr.expresion_value.expresion_array_value->elements[i], scopes);
			}
			break;

		case expresion_func_call_type:
			for (int i = 0; i < expr.expresion_value.expresion_func_call_value->args_count; i++) {
				bytes_count += get_stack_size_of_expr(expr.expresion_value.expresion_func_call_value->args[i], scopes);
			}
			break;
	}
	return bytes_count;
}

// returns the amount of memory that the scope uses in the stack
static int get_stack_size_of_scope(const Node_Scope scope, ASM_Scopes_List * scopes) {
	int stack_size_bytes = 0;
	int max_expr_size_bytes = 0;
	for (int i = 0; i < scope.statements_count; i++) {
		Node_Statement stmt = scope.statements_node[i];
		switch (stmt.statement_type) {
			case exit_node_type:
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.exit_node.exit_code, *scopes));
				break;

			case var_assignment_type:
				if (stmt.statement_value.var_assignment.destination.destination_type == assgn_dest_deref_type) {
					max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(*stmt.statement_value.var_assignment.destination.destination_value.deref_expresion, *scopes));
				} else if (stmt.statement_value.var_assignment.destination.destination_type == assgn_dest_subscript_type) {
					max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(*stmt.statement_value.var_assignment.destination.destination_value.index_expr, *scopes));
				}
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.var_assignment.value, *scopes));
				break;

			case print_type:
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.print.chr, *scopes));
				break;

			case func_def_type:
				implementation_error("can not get the size of a function declaration inside of a scope");
				break;

			case return_type:
				if (stmt.statement_value.return_node.returns_value) {
					max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.return_node.ret_value, *scopes));
				}
				break;

			case var_declaration_type:
				NASM_append_var_to_var_list(scopes, stmt.statement_value.var_declaration.var_name, (ASM_Memory_place){0}, (Node_Type){0});
				stack_size_bytes += get_size_of_type(stmt.statement_value.var_declaration.type);
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.var_declaration.value, *scopes));
				break;

			case scope_type:
				stack_size_bytes += get_stack_size_of_scope(stmt.statement_value.scope, scopes);
				break;

			case if_type:
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.if_node.condition, *scopes));
				int if_stack_size = get_stack_size_of_scope(stmt.statement_value.if_node.scope, scopes);
				if (stmt.statement_value.if_node.has_else_block) {
					if_stack_size = MAX(if_stack_size, get_stack_size_of_scope(stmt.statement_value.if_node.else_block, scopes));
				}
				stack_size_bytes += if_stack_size;
				break;

			case while_type:
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.while_node.condition, *scopes));
				stack_size_bytes += get_stack_size_of_scope(stmt.statement_value.while_node.scope, scopes);
				break;

			case expresion_stmt_type:
				max_expr_size_bytes = MAX(max_expr_size_bytes, get_stack_size_of_expr(stmt.statement_value.expresion_stmt, *scopes));
				break;
		}
	}
	return stack_size_bytes + max_expr_size_bytes;
}

// generates asm from expresion the result will be put in the top of the stack
static void gen_NASM_expresion(FILE * file_ptr, const Node_Expresion expresion, int stack_size, const ASM_Scopes_List vars) {
	switch (expresion.expresion_type) {
		case expresion_number_type:
			add_string_to_file(file_ptr, "mov qword [rbp - ");
			fprintf(file_ptr, "%d", stack_size);
			add_string_to_file(file_ptr, "], ");
			add_token_to_file(file_ptr, expresion.expresion_value.expresion_number_value);
			add_string_to_file(file_ptr, "\n");
			stack_size += U64_sz;
			break;

		case expresion_identifier_type:
			Token identifier = expresion.expresion_value.expresion_identifier_value;
			Node_Type indentifier_type = NASM_get_type_of_expresion(expresion, vars);
			if (indentifier_type.type_type == type_array_type) {
				if (get_size_of_type(*indentifier_type.type_value.type_array_value->primitive_type) != U64_sz) {
					implementation_error("generating an array with non u64 type is not implemented in ASM");
				}
				ASM_Memory_place array_stack_place = find_var_mem_place(vars, identifier);
				int array_elements_count = number_token_to_int(indentifier_type.type_value.type_array_value->elements_count);
				// copy the array in reverse order to the top of the stack
				// keep the address of the array
				for (int i = 0; i < array_elements_count; i++) {
					// read bytes
					if (array_stack_place.place_type == stack_place_type) {
						add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
						fprintf(file_ptr, "%d", array_stack_place.place_value.stack_place_value + i*U64_sz);
						add_string_to_file(file_ptr, "]\n");
					} else if (array_stack_place.place_type == arg_place_type) {
						add_string_to_file(file_ptr, "mov rax, qword [rbp + ");
						fprintf(file_ptr, "%d", array_stack_place.place_value.arg_place_value + get_size_of_type(indentifier_type) - i*U64_sz);
						add_string_to_file(file_ptr, "]\n");
					} else {
						implementation_error("tried to generate an array that is not in the stack nor it is an function argument");
					}
					// write bytes
					add_string_to_file(file_ptr, "mov qword [rbp - ");
					fprintf(file_ptr, "%d", stack_size + i*U64_sz);
					add_string_to_file(file_ptr, "], rax\n");
				}
				stack_size += array_elements_count*U64_sz;
			} else {
				ASM_Memory_place var_mem_place = find_var_mem_place(vars, identifier);
				// put variable into 'rax'
				if (var_mem_place.place_type == stack_place_type) {
					add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
					fprintf(file_ptr, "%d", var_mem_place.place_value.stack_place_value);
					add_string_to_file(file_ptr, "]\n");
				} else if (var_mem_place.place_type == arg_place_type) {
					add_string_to_file(file_ptr, "mov rax, qword [rbp + ");
					fprintf(file_ptr, "%d", var_mem_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(identifier, vars)));
					add_string_to_file(file_ptr, "]\n");
				} else if (var_mem_place.place_type == string_place_type) {
					add_string_to_file(file_ptr, "mov rax, ");
					fprintf(file_ptr, "%s", var_mem_place.place_value.string_place_value);
					add_string_to_file(file_ptr, "\n");
				}
				// put rax onto the stack
				add_string_to_file(file_ptr, "mov qword [rbp - ");
				fprintf(file_ptr, "%d", stack_size);
				add_string_to_file(file_ptr, "], rax\n");
				stack_size += get_size_of_type(indentifier_type);
			}
			break;

		case expresion_binary_operation_type:
			// TODO: this should be handled in switch case like the other operations 
			if (expresion.expresion_value.expresion_binary_operation_value->operation_type == binary_operation_access_type) {
				int old_stack_size = stack_size;
				// put the array onto the stack top
				int array_addr = stack_size;
				int array_element_size = get_size_of_type(*NASM_get_type_of_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side, vars).type_value.type_array_value->primitive_type);
				int array_size = get_size_of_type(NASM_get_type_of_expresion(expresion.expresion_value.expresion_binary_operation_value->left_side, vars));
				gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->left_side, stack_size, vars);
				stack_size += array_size;

				// put the index onto the stack
				int index_addr = stack_size;
				gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->right_side, stack_size, vars);
				stack_size += 8;
				// load the address of the array
				add_string_to_file(file_ptr, "lea rax, [rbp - ");
				fprintf(file_ptr, "%d", array_addr+array_size-array_element_size);
				add_string_to_file(file_ptr, "]\n");
				// load the index
				add_string_to_file(file_ptr, "mov rbx, qword [rbp - ");
				fprintf(file_ptr, "%d", index_addr);
				add_string_to_file(file_ptr, "]\n");
				// adjust the index to the size of the type inside the array
				add_string_to_file(file_ptr, "lea rbx, [rbx * ");
				fprintf(file_ptr, "%d", U64_sz);
				add_string_to_file(file_ptr, "]\n");
				// get the value at the index in the array
				add_string_to_file(file_ptr, "mov rax, qword [rax+rbx]\n");
				// put the result in stack top
				add_string_to_file(file_ptr, "mov qword [rbp - ");
				fprintf(file_ptr, "%d", old_stack_size);
				add_string_to_file(file_ptr, "], rax\n");

				return;
			}
			int old_stack_size = stack_size;
			// put left hand side expresion into stack top
			stack_size += 8;
			gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->left_side, stack_size, vars);
			int lhs_stack_place = stack_size;
			// put also the right side into the stack
			stack_size += 8;
			gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_binary_operation_value->right_side, stack_size, vars);
			int rhs_stack_place = stack_size;
			// load the first operand
			add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
			fprintf(file_ptr, "%d", lhs_stack_place);
			add_string_to_file(file_ptr, "]\n");
			// load the second operand
			add_string_to_file(file_ptr, "mov rbx, qword [rbp - ");
			fprintf(file_ptr, "%d", rhs_stack_place);
			add_string_to_file(file_ptr, "]\n");
			// perform the corresponding binary operation
			switch (expresion.expresion_value.expresion_binary_operation_value->operation_type) {
					case binary_operation_sum_type:
						// add them together
						add_string_to_file(file_ptr, "add rax, rbx\n");
						// put the result into the stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case binary_operation_sub_type:
						// substruct them
						add_string_to_file(file_ptr, "sub rax, rbx\n");
						// put the result in stack topo
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case binary_operation_mul_type:
						// multiply them
						add_string_to_file(file_ptr, "mul rbx\n"); // NOTE: this changes rdx
						// put the result in stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case binary_operation_div_type:
						// set rdx to 0 because the dividend is the extended register rdx:rax
						add_string_to_file(file_ptr, "xor rdx, rdx\n");
						// divide them
						add_string_to_file(file_ptr, "div rbx\n");
						// put the result in stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case binary_operation_mod_type:
						// set rdx to 0 because the 'dividend' is the extended register rdx:rax
						add_string_to_file(file_ptr, "xor rdx, rdx\n");
						// perform the modulo
						add_string_to_file(file_ptr, "div rbx\n");
						// put the result in stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rdx\n");
						break;

					case binary_operation_equ_type:
						// if rax and rbx are equal set rax to 1 otherwise to 0
						add_string_to_file(file_ptr, "cmp rax, rbx\n");
						add_string_to_file(file_ptr, "sete al\n");
						add_string_to_file(file_ptr, "movzx rax, al\n");
						// put the result in stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case binary_operation_big_type:
						// if rax and rbx are equal set rax to 1 otherwise to 0
						add_string_to_file(file_ptr, "cmp rax, rbx\n");
						add_string_to_file(file_ptr, "seta al\n");
						add_string_to_file(file_ptr, "movzx rax, al\n");
						// put the result in stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case binary_operation_les_type:
						// if rax and rbx are equal set rax to 1 otherwise to 0
						add_string_to_file(file_ptr, "cmp rax, rbx\n");
						add_string_to_file(file_ptr, "setb al\n");
						add_string_to_file(file_ptr, "movzx rax, al\n");
						// put the result in stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", old_stack_size);
						add_string_to_file(file_ptr, "], rax\n");
						break;
				}
				break;

		case expresion_unary_operation_type:
			// perform the corresponding unary operation
			switch (expresion.expresion_value.expresion_unary_operation_value->operation_type) {
					case unary_operation_addr_type:
						stack_size += PTR_sz;
						ASM_Memory_place var_mem_place = find_var_mem_place(vars, expresion.expresion_value.expresion_unary_operation_value->expresion.expresion_value.expresion_identifier_value);
						// get the address of a variable
						if (var_mem_place.place_type == stack_place_type) {
							add_string_to_file(file_ptr, "lea rax, [rbp - ");
							fprintf(file_ptr, "%d", var_mem_place.place_value.stack_place_value);
							add_string_to_file(file_ptr, "]\n");
						} else if (var_mem_place.place_type == arg_place_type) {
							add_string_to_file(file_ptr, "lea rax, [rbp + ");
							fprintf(file_ptr, "%d", var_mem_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(expresion.expresion_value.expresion_unary_operation_value->expresion.expresion_value.expresion_identifier_value, vars)));
							add_string_to_file(file_ptr, "]\n");
						} else {
							implementation_error("tried to generate a dereference to a variable that is not in the stack nor is it an function argument");
						}
						// put the result into the stack top
						add_string_to_file(file_ptr, "mov qword [rbp - ");
						fprintf(file_ptr, "%d", stack_size-PTR_sz);
						add_string_to_file(file_ptr, "], rax\n");
						break;

					case unary_operation_deref_type:
						int beginning_stack_size = stack_size;
						// put the pointer/expression onto the stack
						gen_NASM_expresion(file_ptr, expresion.expresion_value.expresion_unary_operation_value->expresion, stack_size, vars);
						int expresion_type_size = get_size_of_type(*NASM_get_type_of_expresion(expresion.expresion_value.expresion_unary_operation_value->expresion, vars).type_value.type_ptr_value);
						stack_size += expresion_type_size;

						// get the expression the pointer
						add_string_to_file(file_ptr, "mov rbx, qword [rbp - ");
						fprintf(file_ptr, "%d", beginning_stack_size);
						add_string_to_file(file_ptr, "]\n");
						// dereference the pointer and put the expression onto the stack
						for (int i = 0; i < expresion_type_size; i++) {
							add_string_to_file(file_ptr, "mov al, byte [rbx-");
							fprintf(file_ptr, "%d", i);
							add_string_to_file(file_ptr, "]\n");
						
							add_string_to_file(file_ptr, "mov byte [rbp-");
							fprintf(file_ptr, "%d", beginning_stack_size + i);
							add_string_to_file(file_ptr, "], al\n");
						}
						break;
				}
				break;

		case expresion_array_type:;
			Node_Array array = *expresion.expresion_value.expresion_array_value;
			int single_element_size = get_size_of_type(NASM_get_type_of_expresion(array.elements[0], vars));
			// generate every element in the array
			for (int i = array.elements_count -1; i >= 0; i--) {
				gen_NASM_expresion(file_ptr, array.elements[i], stack_size, vars);
				// progresively allocate space for the element in each iteration
				stack_size += single_element_size;
			}
			break;
		
		case expresion_func_call_type:
			Node_Func_call func_call = *expresion.expresion_value.expresion_func_call_value;
			int func_begin_stack = stack_size;
			Node_Func_type func_type = *NASM_get_type_of_variable(func_call.func_name, vars).type_value.type_func_value;
			int args_size = 0;
			if (func_type.returns_value && get_size_of_type(func_type.return_type) > U64_sz) {
				// allocate space in the stack for the return value
				stack_size += get_size_of_type(func_type.return_type);
				args_size += get_size_of_type(func_type.return_type);
				add_string_to_file(file_ptr, "sub rsp, ");
				fprintf(file_ptr, "%d", get_size_of_type(func_type.return_type));
				add_string_to_file(file_ptr, "\n");
				// append an argument with the address of the allocated space
				add_string_to_file(file_ptr, "lea rax, [rbp - ");
				fprintf(file_ptr, "%d", stack_size);
				add_string_to_file(file_ptr, "]\n");
				add_string_to_file(file_ptr, "push rax\n");
				stack_size += U64_sz;
				args_size += U64_sz;
			}
			for (int i = func_call.args_count -1; i >= 0; i--) {
				int arg_size = get_size_of_type(NASM_get_type_of_expresion(func_call.args[i], vars));
				stack_size += arg_size;
				args_size += arg_size;
				gen_NASM_expresion(file_ptr, func_call.args[i], stack_size, vars);
				if (arg_size % U64_sz) {
					implementation_error("can not pass an argument which size is not a multiple of 8");
				}
				for (int j = 0; j < arg_size; j += U64_sz) {
					add_string_to_file(file_ptr, "mov rax, qword [rbp - ");
					fprintf(file_ptr, "%d", stack_size + j);
					add_string_to_file(file_ptr, "]\n");
					add_string_to_file(file_ptr, "push rax\n");
				}
			}
			ASM_Memory_place func_mem_place = find_var_mem_place(vars, func_call.func_name);
			if (func_mem_place.place_type == stack_place_type) {
				add_string_to_file(file_ptr, "call qword [rbp - ");
				fprintf(file_ptr, "%d", func_mem_place.place_value.stack_place_value);
				add_string_to_file(file_ptr, "]\n");
			} else if (func_mem_place.place_type == arg_place_type) {
				add_string_to_file(file_ptr, "call qword [rbp + ");
				fprintf(file_ptr, "%d", func_mem_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(func_call.func_name, vars)));
				add_string_to_file(file_ptr, "]\n");
			} else {
				add_string_to_file(file_ptr, "call ");
				add_token_to_file(file_ptr, func_call.func_name);
				add_string_to_file(file_ptr, "\n");
			}
			add_string_to_file(file_ptr, "add rsp, ");
			fprintf(file_ptr, "%d", args_size);
			add_string_to_file(file_ptr, "\n");
			// if the return value fits in 'rax' it will be put there otherwise
			// the function already has put the return value onto the stack
			// and dont return a value if the function does not return a value ofc
			if (func_type.returns_value && get_size_of_type(func_type.return_type) <= U64_sz) {
				add_string_to_file(file_ptr, "mov qword [rbp - ");
				fprintf(file_ptr, "%d", func_begin_stack);
				add_string_to_file(file_ptr, "], rax\n");
			}
			break;
	}
}

// keep track of an unique identification for the labels so there arent collisions with other labels
static int uuid = 0;


static void gen_NASM_statement(FILE * out_file_ptr, ASM_Scopes_List * variables, const Node_Statement stmt, int * stack_size);


static void gen_NASM_var_declaration(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_Var_declaration var_declaration, int * stack_size) {
	int expresion_size = get_size_of_type(var_declaration.type);
	gen_NASM_expresion(out_file_ptr, var_declaration.value, *stack_size, *variables);
	// add the location of the expression to the list of vars
	ASM_Memory_place var_mem_place;
	var_mem_place.place_type = stack_place_type;
	var_mem_place.place_value.stack_place_value = *stack_size;
	NASM_append_var_to_var_list(variables, var_declaration.var_name, var_mem_place, var_declaration.type);
	// allocate space for value in stack
	*stack_size += expresion_size;
}

static void gen_NASM_exit_node(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_Exit exit_node, int * stack_size) {
	// NOTE: this only works for unix-like OSes
	gen_NASM_expresion(out_file_ptr, exit_node.exit_code, *stack_size, *variables);
	add_string_to_file(out_file_ptr, "mov rax, 60\n");
	add_string_to_file(out_file_ptr, "mov rdi, qword [rbp - ");
	fprintf(out_file_ptr, "%d", *stack_size);
	add_string_to_file(out_file_ptr, "]\n");
	add_string_to_file(out_file_ptr, "syscall\n");
}

static void gen_NASM_scope(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_Scope scope, int * stack_size) {
	NASM_create_scope(variables);
	int temp_stack_size = *stack_size;
	for (int i = 0; i < scope.statements_count; i++) {
		gen_NASM_statement(out_file_ptr, variables, scope.statements_node[i], &temp_stack_size);
	}
	NASM_remove_scope(variables);
}

static void gen_NASM_var_assignment(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_Var_assignment var_assignment, int * stack_size) {
	int init_stack_size = *stack_size;
	switch (var_assignment.destination.destination_type) {
		case assgn_dest_var_name_type:
			Token var_name = var_assignment.destination.destination_value.var_name;
			// generate the expression to be assigned
			gen_NASM_expresion(out_file_ptr, var_assignment.value, *stack_size, *variables);
			if (NASM_get_type_of_expresion(var_assignment.value, *variables).type_type == type_array_type) {
				int arr_elements_count = number_token_to_int(NASM_get_type_of_expresion(var_assignment.value, *variables).type_value.type_array_value->elements_count);
				for (int i = 0; i < arr_elements_count; i++) {
					// get the expression from the top of the stack
					add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
					fprintf(out_file_ptr, "%d", *stack_size + i*U64_sz);
					add_string_to_file(out_file_ptr, "]\n");
					// assign the value of the expresion from the stack to the variable element by element
					ASM_Memory_place var_mem_place = find_var_mem_place(*variables, var_name);
					if (var_mem_place.place_type == stack_place_type) {
						add_string_to_file(out_file_ptr, "mov qword [rbp - ");
						fprintf(out_file_ptr, "%d", var_mem_place.place_value.stack_place_value + i*U64_sz);
						add_string_to_file(out_file_ptr, "], rax\n");
					} else if (var_mem_place.place_type == arg_place_type) {
						add_string_to_file(out_file_ptr, "mov qword [rbp + ");
						fprintf(out_file_ptr, "%d", var_mem_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(var_name, *variables)) - i*U64_sz);
						add_string_to_file(out_file_ptr, "], rax\n");
					}
				}
			} else {
				// get the expression from the top of the stack
				add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
				fprintf(out_file_ptr, "%d", *stack_size);
				add_string_to_file(out_file_ptr, "]\n");
				// assign the value of the expresion from the stack to the variable
				ASM_Memory_place var_mem_place = find_var_mem_place(*variables, var_name);
				if (var_mem_place.place_type == stack_place_type) {
					add_string_to_file(out_file_ptr, "mov qword [rbp - ");
					fprintf(out_file_ptr, "%d", var_mem_place.place_value.stack_place_value);
					add_string_to_file(out_file_ptr, "], rax\n");
				} else if (var_mem_place.place_type == arg_place_type) {
					add_string_to_file(out_file_ptr, "mov qword [rbp + ");
					fprintf(out_file_ptr, "%d", var_mem_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(var_name, *variables)));
					add_string_to_file(out_file_ptr, "], rax\n");
				}
			}
			break;

		case assgn_dest_deref_type:
			Node_Expresion dest_expr = *var_assignment.destination.destination_value.deref_expresion;
			// generate the expression to be assigned
			gen_NASM_expresion(out_file_ptr, var_assignment.value, *stack_size, *variables);
			// generate the destination expression
			gen_NASM_expresion(out_file_ptr, dest_expr, *stack_size + U64_sz, *variables);
			add_string_to_file(out_file_ptr, "mov rdi, qword [rbp - ");
			fprintf(out_file_ptr, "%d", *stack_size + U64_sz);
			add_string_to_file(out_file_ptr, "]\n");
			// get the source expression from the stack
			add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
			fprintf(out_file_ptr, "%d", init_stack_size);
			add_string_to_file(out_file_ptr, "]\n");
			// assign the value of the expresion from the stack to the destination pointer
			add_string_to_file(out_file_ptr, "mov qword [rdi], rax");
			break;

		case assgn_dest_subscript_type:
			Token array_name = var_assignment.destination.destination_value.array_name;
			Node_Expresion index_expr = *var_assignment.destination.destination_value.index_expr;
			Node_Type array_type = NASM_get_type_of_variable(array_name, *variables);
			int array_elements_count = number_token_to_int(array_type.type_value.type_array_value->elements_count);
			int array_element_size = get_size_of_type(array_type.type_value.type_array_value->primitive_type[0]);
			// generate the expression to be assigned
			gen_NASM_expresion(out_file_ptr, var_assignment.value, *stack_size, *variables);
			// generate the index
			gen_NASM_expresion(out_file_ptr, index_expr, *stack_size + U64_sz, *variables);
			add_string_to_file(out_file_ptr, "mov rdi, qword [rbp - ");
			fprintf(out_file_ptr, "%d", *stack_size + U64_sz);
			add_string_to_file(out_file_ptr, "]\n");
			ASM_Memory_place array_mem_place = find_var_mem_place(*variables, array_name);
			// get the address of the indexed element in the array
			if (array_mem_place.place_type == stack_place_type) {
				add_string_to_file(out_file_ptr, "lea rdi, [rbp - ");
				fprintf(out_file_ptr, "%d", array_mem_place.place_value.stack_place_value + (array_elements_count -1)*array_element_size);
				add_string_to_file(out_file_ptr, " + rdi*");
				fprintf(out_file_ptr, "%d", U64_sz);
				add_string_to_file(out_file_ptr, "]\n");
			} else if (array_mem_place.place_type == stack_place_type) {
				// FIX: maybe in an array the index has to be treated differently
				add_string_to_file(out_file_ptr, "lea rdi, [rbp + ");
				fprintf(out_file_ptr, "%d", array_mem_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(array_name, *variables)) + (array_elements_count -1)*array_element_size);
				add_string_to_file(out_file_ptr, " + rdi*");
				fprintf(out_file_ptr, "%d", U64_sz);
				add_string_to_file(out_file_ptr, "]\n");
			} else {
				implementation_error("tried to generate an array that is not in the stack or is a function argument");
			}
			// assign the value of the expresion to the array element
			add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
			fprintf(out_file_ptr, "%d", init_stack_size);
			add_string_to_file(out_file_ptr, "]\n");
			add_string_to_file(out_file_ptr, "mov qword [rdi], rax\n");
			break;
	}
}

static void gen_NASM_print(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_Print print_node, int * stack_size) {
	// NOTE: this only works for unix-like OSes
	gen_NASM_expresion(out_file_ptr, print_node.chr, *stack_size, *variables);
	add_string_to_file(out_file_ptr, "lea rsi, [rbp - ");
	fprintf(out_file_ptr, "%d", *stack_size);
	add_string_to_file(out_file_ptr, "]\n");
	add_string_to_file(out_file_ptr, "mov rdx, 1\n"); // symbols to print
	add_string_to_file(out_file_ptr, "mov rdi, 1\n"); // std output
	add_string_to_file(out_file_ptr, "mov rax, 1\n"); // write syscall
	add_string_to_file(out_file_ptr, "syscall\n");
}

static void gen_NASM_if_node(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_If if_node, int * stack_size) {
	// generate the condition
	Node_Expresion condition = if_node.condition;
	gen_NASM_expresion(out_file_ptr, condition, *stack_size, *variables);

	// if the condition is not true skip the if body
	add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
	fprintf(out_file_ptr, "%d", *stack_size);
	add_string_to_file(out_file_ptr, "]\n");
	add_string_to_file(out_file_ptr, "test rax, rax\n");
	int if_uid = uuid; // save the uid in case it gets modified in the scope
	uuid++;
	// if the condition is not met skip the `if` block
	fprintf(out_file_ptr, "jz .IF%d\n", if_uid);

	// generate the `if` scope
	NASM_create_scope(variables);
	int tmp_stack_sz = *stack_size;
	for (int i = 0; i < if_node.scope.statements_count; i++) {
		gen_NASM_statement(out_file_ptr, variables, if_node.scope.statements_node[i], &tmp_stack_sz);
	}
	NASM_remove_scope(variables);

	if (if_node.has_else_block) {
		// if the `if` block is executed skip the `else` block
		fprintf(out_file_ptr, "jmp .EL%d\n", if_uid);
	}
	// generate the `if` label
	fprintf(out_file_ptr, ".IF%d:\n", if_uid);
	if (if_node.has_else_block) {
		// generate `else` block code
		NASM_create_scope(variables);
		int tmp_stack_sz = *stack_size;
		for (int i = 0; i < if_node.else_block.statements_count; i++) {
			gen_NASM_statement(out_file_ptr, variables, if_node.else_block.statements_node[i], &tmp_stack_sz);
		}
		NASM_remove_scope(variables);
		fprintf(out_file_ptr, ".EL%d:\n", if_uid); // generate the `else` label
	}
}

static void gen_NASM_while_node(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_While while_node, int * stack_size) {
	int while_uid = uuid; // save the uid in case it gets modified in the scope
	uuid++;
	// generate the label for repeating the loop
	fprintf(out_file_ptr, ".WHB%d:\n", while_uid); // WHB is for "while beginning"
	// generate the condition
	Node_Expresion condition = while_node.condition;
	gen_NASM_expresion(out_file_ptr, condition, *stack_size, *variables);

	// if the condition is not true skip the while body
	add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
	fprintf(out_file_ptr, "%d", *stack_size);
	add_string_to_file(out_file_ptr, "]\n");
	add_string_to_file(out_file_ptr, "test rax, rax\n");
	fprintf(out_file_ptr, "jz .WHE%d\n", while_uid); // WHE is for "while end"

	// generate the scope
	NASM_create_scope(variables);
	int temp_stack_size = *stack_size;
	for (int i = 0; i < while_node.scope.statements_count; i++) {
		gen_NASM_statement(out_file_ptr, variables, while_node.scope.statements_node[i], &temp_stack_size);
	}
	NASM_remove_scope(variables);

	fprintf(out_file_ptr, "jmp .WHB%d\n", while_uid);
	fprintf(out_file_ptr, ".WHE%d:\n", while_uid); // generate the label for finnishing the while loop
}

static void gen_NASM_return(FILE * out_file_ptr, ASM_Scopes_List * variables, Node_Return return_node, int * stack_size) {
	if (return_node.returns_value) {
		gen_NASM_expresion(out_file_ptr, return_node.ret_value, *stack_size, *variables);
		if (check_if_var_declared(*variables, "$RET_ADDR")) {
			// FIX: this leaks memory and is horrible
			Token ret_var_token;
			ret_var_token.beginning = smalloc(sizeof("$RET_ADDR")-1);
			memcpy(ret_var_token.beginning, "$RET_ADDR", sizeof("$RET_ADDR")-1);
			ret_var_token.length = sizeof("$RET_ADDR")-1;
			ASM_Memory_place ret_var_place = find_var_mem_place(*variables, ret_var_token);
			if (ret_var_place.place_type != arg_place_type) {
				implementation_error("the hidder return address is not stores in the stack");
			}
			add_string_to_file(out_file_ptr, "mov rdi, qword [rbp + ");
			fprintf(out_file_ptr, "%d", ret_var_place.place_value.arg_place_value + get_size_of_type(NASM_get_type_of_variable(ret_var_token, *variables)));
			add_string_to_file(out_file_ptr, "]\n");
			// copy every byte of the top of the stack from the expresion into the return address
			int expresion_size_bytes = get_size_of_type(NASM_get_type_of_expresion(return_node.ret_value, *variables));
			for (int i = 0; i < expresion_size_bytes; i++) {
				// read byte from the stack
				add_string_to_file(out_file_ptr, "mov al, byte [rbp - ");
				fprintf(out_file_ptr, "%d", *stack_size + i);
				add_string_to_file(out_file_ptr, "]\n");
				// write byte to return addr
				add_string_to_file(out_file_ptr, "mov byte [rdi+");
				fprintf(out_file_ptr, "%d", expresion_size_bytes-i);
				add_string_to_file(out_file_ptr, "], al\n");
			}
		} else {
			add_string_to_file(out_file_ptr, "mov rax, qword [rbp - ");
			fprintf(out_file_ptr, "%d", *stack_size);
			add_string_to_file(out_file_ptr, "]\n");
		}
	}
	add_string_to_file(out_file_ptr, "jmp .FN_RET\n");
}


static void gen_NASM_statement(FILE * out_file_ptr, ASM_Scopes_List * variables, const Node_Statement stmt, int * stack_size) {
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
		
		case func_def_type:
			// the function have already been generated before
			return;
		
		case return_type:
			Node_Return return_node = stmt.statement_value.return_node;
			gen_NASM_return(out_file_ptr, variables, return_node, stack_size);
			break;
		
		case expresion_stmt_type:
			Node_Expresion expresion = stmt.statement_value.expresion_stmt;
			gen_NASM_expresion(out_file_ptr, expresion, *stack_size, *variables);
			break;
	}
	add_string_to_file(out_file_ptr, "\n");
}

// it generates NASM code
void gen_NASM_code(const Node_Program syntax_tree, const char * out_file_name) {
	FILE * out_file_ptr = create_file(out_file_name);

	add_string_to_file(out_file_ptr, "bits 64\n"); // targeting 64 bits
	add_string_to_file(out_file_ptr, "default rel\n"); // make all the pointers `rip` based
	add_string_to_file(out_file_ptr, "global _start\n\n"); // needed for linking in ELF format

	// this will hold all the variables from all the scopes
	ASM_Scopes_List scopes;
	scopes.scopes_count = 0;
	scopes.variables = malloc(scopes.scopes_count * sizeof(ASM_Scopes_List));
	NASM_create_scope(&scopes); // create first global scope
	int stack_size = 0;

	// generate the functions
	for (int i = 0; i < syntax_tree.statements_count; i++) {
		if (syntax_tree.statements_node[i].statement_type == func_def_type) {
			Node_Func_def func = syntax_tree.statements_node[i].statement_value.func_def;
			// append the function to the list of symbols
			ASM_Memory_place func_mem_place;
			func_mem_place.place_type = string_place_type;
			func_mem_place.place_value.string_place_value = smalloc((func.name.length+1) * sizeof(*func_mem_place.place_value.string_place_value));
			memcpy(func_mem_place.place_value.string_place_value, func.name.beginning, func.name.length);
			func_mem_place.place_value.string_place_value[func.name.length] = '\0';
			NASM_append_var_to_var_list(&scopes, func.name, func_mem_place, func.type);
			// create a scope to define the function arguments in it
			NASM_create_scope(&scopes);
			// leave the space on the stack for 'rip' free
			int args_size_count = PTR_sz;
			for (int j = 0; j < func.type.type_value.type_func_value->args_count; j++) {
				ASM_Memory_place arg_mem_place;
				arg_mem_place.place_type = arg_place_type;
				arg_mem_place.place_value.arg_place_value = args_size_count;
				args_size_count += get_size_of_type(func.type.type_value.type_func_value->args_type[j]);
				NASM_append_var_to_var_list(&scopes, func.type.type_value.type_func_value->args_name[j], arg_mem_place, func.type.type_value.type_func_value->args_type[j]);
			}
			if (func.type.type_value.type_func_value->returns_value && get_size_of_type(func.type.type_value.type_func_value->return_type) > U64_sz) {
				// the name of the hidden function argument
				const char ret_var_name[] = "$RET_ADDR";

				ASM_Memory_place ret_var_place;
				ret_var_place.place_type = arg_place_type;
				ret_var_place.place_value.arg_place_value = args_size_count;

				Token ret_var_token;
				ret_var_token.beginning = smalloc(sizeof(ret_var_name)-1);
				memcpy(ret_var_token.beginning, ret_var_name, sizeof(ret_var_name)-1);
				ret_var_token.length = sizeof(ret_var_name)-1;

				Node_Type ret_var_type;
				ret_var_type.token.beginning = smalloc(4);
				memcpy(ret_var_type.token.beginning, "u64", 4);
				ret_var_type.type_type = type_primitive_type;
				ret_var_type.type_value.type_primitive_value = ret_var_type.token;
				// declare the hidden argument as if it was just a normal variable
				NASM_append_var_to_var_list(&scopes, ret_var_token, ret_var_place, ret_var_type);
				// append this argument to the list of arguments
				func.type.type_value.type_func_value->args_count++;
				func.type.type_value.type_func_value->args_type = srealloc(func.type.type_value.type_func_value->args_type, func.type.type_value.type_func_value->args_count*sizeof(*func.type.type_value.type_func_value->args_type));
				func.type.type_value.type_func_value->args_type[func.type.type_value.type_func_value->args_count -1] = ret_var_type;
				func.type.type_value.type_func_value->args_name = srealloc(func.type.type_value.type_func_value->args_name, func.type.type_value.type_func_value->args_count*sizeof(*func.type.type_value.type_func_value->args_name));
				func.type.type_value.type_func_value->args_name[func.type.type_value.type_func_value->args_count -1] = ret_var_token;
			}
			// initialize the function stack
			// TODO: creating a scope just to get the size of the stack is crappy. change it?
			NASM_create_scope(&scopes);
			int func_stack_used = get_stack_size_of_scope(func.scope, &scopes);
			NASM_remove_scope(&scopes);
			add_token_to_file(out_file_ptr, func.name);
			add_string_to_file(out_file_ptr, ":\n");
			add_string_to_file(out_file_ptr, "\tpush rbp\n");
			add_string_to_file(out_file_ptr, "\tmov rbp, rsp\n");
			fprintf(out_file_ptr, "\tsub rsp, %d\n", func_stack_used);
			// leave the space on the stack for 'rip' free
			int func_stack_size = PTR_sz;
			gen_NASM_scope(out_file_ptr, &scopes, func.scope, &func_stack_size);
			NASM_remove_scope(&scopes);
			// exit the function
			add_string_to_file(out_file_ptr, ".FN_RET:\n");
			fprintf(out_file_ptr, "\tadd rsp, %d\n", func_stack_used);
			add_string_to_file(out_file_ptr, "\tpop rbp\n");
			add_string_to_file(out_file_ptr, "\tret\n\n");
		}
	}

	add_string_to_file(out_file_ptr, "_start:\n");
	add_string_to_file(out_file_ptr, "mov rbp, rsp\n");
	add_string_to_file(out_file_ptr, "sub rsp, 0x1000\n\n"); // FIX: remove

	for (int i = 0; i < syntax_tree.statements_count; i++) {
		Node_Statement node = syntax_tree.statements_node[i];
		gen_NASM_statement(out_file_ptr, &scopes, node, &stack_size);
	}

	NASM_free_scopes_list(scopes);

	// exit the program safely with a syscall
	// NOTE: OS dependent
	add_string_to_file(out_file_ptr, "mov rax, 60\n");
	add_string_to_file(out_file_ptr, "xor rdi, rdi\n");
	add_string_to_file(out_file_ptr, "syscall\n");

	fclose(out_file_ptr);
}


#endif