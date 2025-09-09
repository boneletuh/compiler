#ifndef CHECKER_H_
#define CHECKER_H_

#include "errors.h"
#include "mlib.h"
#include "tokenizer.h"
#include "parser.h"

// TODO: use hashmap instead of array
// array with the variables inside a scope
typedef struct Symbol {
	Token value;
	Node_Type type;
} Symbol;

typedef struct Symbols_scope {
	int vars_count;
	Token associated_var;
	Symbol * vars;
} Symbols_scope;

// array with all the scopes
typedef struct Symbol_table {
	int scopes_count;
	Symbols_scope * scopes;
} Symbol_table;

bool is_checking_func_def = false;

// returns if the 2 types are equal
static bool compare_2_types(const Node_Type type1, const Node_Type type2) {
	if (type1.type_type != type2.type_type) {
		return false;
	}
	// TODO: do this with switch
	// does not matter if it is type1.type_type or type2.type_type
	if (type1.type_type == type_primitive_type) {
		Token token_type1 = type1.type_value.type_primitive_value;
		Token token_type2 = type2.type_value.type_primitive_value;
		return compare_str_of_tokens(token_type1, token_type2);
	}
	else if (type1.type_type == type_ptr_type) {
		Node_Type tmp_type1 = *type1.type_value.type_ptr_value;
		Node_Type tmp_type2 = *type2.type_value.type_ptr_value;
		return compare_2_types(tmp_type1, tmp_type2);
	}
	else if (type1.type_type == type_array_type) {
		Node_Array_type tmp_type1 = *type1.type_value.type_array_value;
		Node_Array_type tmp_type2 = *type2.type_value.type_array_value;
		return compare_2_types(*tmp_type1.primitive_type, *tmp_type2.primitive_type) && compare_str_of_tokens(tmp_type1.elements_count, tmp_type2.elements_count);
	}
	else if (type1.type_type == type_func_type) {
		Node_Func_type func_type1 = *type1.type_value.type_func_value;
		Node_Func_type func_type2 = *type2.type_value.type_func_value;
		if (func_type1.returns_value != func_type2.returns_value){
			return false;
		}
		if (func_type1.returns_value) {
			if (!compare_2_types(func_type1.return_type, func_type2.return_type)) {
				return false;
			}
		}
		if (func_type1.args_count != func_type2.args_count) {
			return false;
		}
		for (int i = 0; i < func_type1.args_count; i++) {
			if (!compare_2_types(func_type1.args_type[i], func_type2.args_type[i])) {
				return false;
			}
		}
	}
	else {
		implementation_error("checking this type of type not implemented");
	}

	return true;
}

// returns the symbol associated with the token
// if it could not find it, it throws an error
static Symbol get_symbol_from_token(const Symbol_table vars, const Token token) {
	for (int i = 0; i < vars.scopes_count; i++) {
		Symbols_scope scope = vars.scopes[i];
		for (int j = 0; j < scope.vars_count; j++) {
			Symbol symbol = scope.vars[j];
			if (compare_str_of_tokens(token, symbol.value)) {
				return symbol;
			}
		}
	}
	implementation_error("could not get the associated symbol from token in checker");
	// unreachable
	return (Symbol) {};
}

// returns the type of a given expresion
Node_Type get_type_of_expresion(const Symbol_table vars, const Node_Expresion expresion) {
	switch (expresion.expresion_type) {
		case expresion_number_type: {
			Node_Type type;
			type.token.beginning = smalloc(4); // FIX: this leaks memory
			strcpy(type.token.beginning, "u64");
			type.token.length = 3;
			type.type_type = type_primitive_type;
			type.type_value.type_primitive_value = type.token;
			return type;
		}
		case expresion_identifier_type: {
			Token identifier = expresion.expresion_value.expresion_identifier_value;
			Symbol symbol = get_symbol_from_token(vars, identifier);
			return symbol.type;
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
				*type.type_value.type_ptr_value = get_type_of_expresion(vars, uni_expresion);
			}
			// if the operation is the dereference operator `*`, the returned type is type the pointer holds
			else if (uni_operation.operation_type == unary_operation_deref_type) {
				type = *get_type_of_expresion(vars, uni_expresion).type_value.type_ptr_value;
			}
			else {
				implementation_error("can not get type of expression unkown unary operator");
			}
			return type;
		}
		case expresion_binary_operation_type: {
			Node_Binary_Operation bin_operation = *expresion.expresion_value.expresion_binary_operation_value;
			Node_Expresion lhs_expr = bin_operation.left_side;
			Node_Expresion rhs_expr = bin_operation.right_side;
			Node_Type lhs_type = get_type_of_expresion(vars, lhs_expr);
			Node_Type rhs_type = get_type_of_expresion(vars, rhs_expr);
			// if any of the operands is a pointer throw an error
			if (lhs_type.type_type == type_ptr_type || rhs_type.type_type == type_ptr_type) {
				error("can not operate with a pointer");
			}
			// if the operation is an array access
			if (bin_operation.operation_type == binary_operation_access_type) {
				// return the type the array contains
				return *lhs_type.type_value.type_array_value->primitive_type;
			}
			// does not matter if its `lhs_type` or `rhs_type` 
			return lhs_type;
		}
		case expresion_array_type: {
			Node_Type type;
			type.token = NULL_TOKEN;
			type.type_type = type_array_type;
			type.type_value.type_array_value = smalloc(sizeof(*type.type_value.type_array_value));
			type.type_value.type_array_value->primitive_type = smalloc(sizeof(*type.type_value.type_array_value->primitive_type));
			*type.type_value.type_array_value->primitive_type = get_type_of_expresion(vars, expresion.expresion_value.expresion_array_value->elements[0]);
			// FIX: convert the number to token in a more reasonable way
			type.type_value.type_array_value->elements_count.beginning = smalloc(6);
			type.type_value.type_array_value->elements_count.length = 6;
			type.type_value.type_array_value->elements_count.beginning[0] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 100000 % 10;
			type.type_value.type_array_value->elements_count.beginning[1] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 10000 % 10;
			type.type_value.type_array_value->elements_count.beginning[2] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 1000 % 10;
			type.type_value.type_array_value->elements_count.beginning[3] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 100 % 10;
			type.type_value.type_array_value->elements_count.beginning[4] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 10 % 10;
			type.type_value.type_array_value->elements_count.beginning[5] = '0' + expresion.expresion_value.expresion_array_value->elements_count / 1 % 10;
			type.type_value.type_array_value->elements_count.type = Number;
			return type;
		}
		case expresion_func_call_type: {
			Node_Type func_type = get_symbol_from_token(vars, expresion.expresion_value.expresion_func_call_value->func_name).type;
			Node_Type func_return_type = func_type.type_value.type_func_value->return_type;
			return func_return_type;
		}
	}
	implementation_error("unkown type of expresion while trying to get its type");
	return (Node_Type) {};
}

// checkes if the token is in the list of variables
static bool is_var_in_var_list(const Symbol_table vars, const Token variable) {
	for (int i = 0; i < vars.scopes_count; i++) {
		Symbols_scope scope = vars.scopes[i];
		for (int j = 0; j < scope.vars_count; j++) {
			Symbol symbol = scope.vars[j];
			if (compare_str_of_tokens(variable, symbol.value)) {
				return true;
			}
		}
	}
	return false;
}

// checks if there is some undeclared var in the expresion, if so it throws an error
static bool is_expresion_valid(const Symbol_table scopes, const Node_Expresion expresion) {
	switch (expresion.expresion_type) {
		case expresion_number_type:
			// does not need to check the number
			break;

		case expresion_identifier_type:
			Token variable = expresion.expresion_value.expresion_identifier_value;
			if (!is_var_in_var_list(scopes, variable)) {
				errorf("Line:%d, column:%d.  Error: undeclared variable used\n", variable.line_number, variable.column_number);
			}
			break;

		case expresion_binary_operation_type:
			Node_Expresion lhs_expr = expresion.expresion_value.expresion_binary_operation_value->left_side;
			Node_Expresion rhs_expr = expresion.expresion_value.expresion_binary_operation_value->right_side;
			Node_Binary_Operation bin_operation = *expresion.expresion_value.expresion_binary_operation_value;
			if (bin_operation.operation_type == binary_operation_access_type) {
				Node_Type lhs_type = get_type_of_expresion(scopes, lhs_expr);
				Node_Type rhs_type = get_type_of_expresion(scopes, rhs_expr);
				// the type for the left side has to be an array and for the right side an 'u64'
				// TODO: make this condition nicer
				if (!(lhs_type.type_type == type_array_type && rhs_type.type_type == type_primitive_type && compare_token_to_string(rhs_type.token, "u64"))) {
					error("can only access an value that has array type, and with an integer index");
				}
			}
			is_expresion_valid(scopes, lhs_expr);
			is_expresion_valid(scopes, rhs_expr);
			break;

		case expresion_unary_operation_type:
			Node_Unary_Operation uni_operation = *expresion.expresion_value.expresion_unary_operation_value;
			if (uni_operation.operation_type == unary_operation_addr_type) {
				// can only get the address of a variable
				if (uni_operation.expresion.expresion_type != expresion_identifier_type) {
					error("can only take the address of a variable");
				}
			}
			else if (uni_operation.operation_type == unary_operation_deref_type) {
				// can only dereference a pointer
				if (get_type_of_expresion(scopes, uni_operation.expresion).type_type != type_ptr_type) {
					error("can only dereference a pointer");
				}
			}
			else {
				implementation_error("unkown type of unary operation while checking");
			}
			is_expresion_valid(scopes, uni_operation.expresion);
			break;

		case expresion_array_type:
			Node_Array array = *expresion.expresion_value.expresion_array_value;
			if (array.elements_count == 0) {
				error("can not have an empty array in expresion");
				return false;
			}

			// check that every expresion inside the array is valid
			for (int i = 0; i < array.elements_count; i++) {
				if (!is_expresion_valid(scopes, array.elements[i])) {
					return false;
				}
			}
			// also check that every expression inside has the same type
			const Node_Type expected_type = get_type_of_expresion(scopes, array.elements[0]); 
			for (int i = 0; i < array.elements_count; i++) {
				Node_Type sub_expresion_type = get_type_of_expresion(scopes, array.elements[i]);
				if (!compare_2_types(expected_type, sub_expresion_type)) {
					errorf("Line:%d, column:%d.  Error: the elements inside the array does not have the same type\n", sub_expresion_type.token.line_number, sub_expresion_type.token.column_number);
				}
			}
			break;
		
		case expresion_func_call_type:
			Node_Func_call func_call = *expresion.expresion_value.expresion_func_call_value;
			if (!is_var_in_var_list(scopes, func_call.func_name)) {
				D_print_token(func_call.func_name);
				errorf("Line:%d, column:%d.  Error: undeclared function used\n", func_call.func_name.line_number, func_call.func_name.column_number);
			}
			Node_Func_type func_type = *get_symbol_from_token(scopes, func_call.func_name).type.type_value.type_func_value;
			if (func_call.args_count != func_type.args_count) {
				errorf("Line:%d, column:%d.  Error: expected %d arguments in function call but got %d\n", func_call.func_name.line_number, func_call.func_name.column_number, func_type.args_count, func_call.args_count);
			}
			for (int i = 0; i < func_call.args_count; i++) {
				is_expresion_valid(scopes, func_call.args[i]);
				// check that the argument passed matches the type defined in the function definition
				Node_Type arg_type = get_type_of_expresion(scopes, func_call.args[i]);
				if (!compare_2_types(func_type.args_type[i], arg_type)) {
					// FIX: allow errorf() to print types in a nice way, so i would be like:
					//      errorf("Line:%d, column:%d.  Error: arg %d has type %T but expected type %T\n", ...);
					errorf("Line:%d, column:%d.  Error: arg %d does not match the type expected defined in the function \"%t\" declaration\n", func_call.func_name.line_number, func_call.func_name.column_number, i, func_call.func_name);
				}
			}
			break;

	}
	return true;
}

// append a variable to the array of variables in the last scope
static void append_var_to_var_list(const Symbol variable, Symbol_table * scopes) {
	Symbols_scope * last_scope = &scopes->scopes[scopes->scopes_count -1];
	last_scope->vars_count++;
	last_scope->vars = srealloc(last_scope->vars, last_scope->vars_count * sizeof(*last_scope->vars));
	last_scope->vars[last_scope->vars_count -1] = variable;
}

// create a new empty scope and append it to the end of array of scopes
static void create_scope(Symbol_table * scopes, const Token associated_var) {
	scopes->scopes_count++;
	scopes->scopes = srealloc(scopes->scopes, scopes->scopes_count * sizeof(*scopes->scopes));
	scopes->scopes[scopes->scopes_count -1].vars_count = 0;
	scopes->scopes[scopes->scopes_count -1].vars = smalloc(scopes->scopes[scopes->scopes_count -1].vars_count * sizeof(*scopes->scopes[scopes->scopes_count -1].vars));
	scopes->scopes[scopes->scopes_count -1].associated_var = associated_var;
}

// remove the last scope from the array of scopes 
static void remove_scope(Symbol_table * scopes) {
	scopes->scopes_count--;
	scopes->scopes = srealloc(scopes->scopes, scopes->scopes_count * sizeof(*scopes->scopes));
}

// free the memory of the scopes
static void free_symbol_table(Symbol_table variables) {
	for (int i = 0; i < variables.scopes_count; i++ ) {
		free(variables.scopes[i].vars);
	}
	free(variables.scopes);
}

// get the last associated var to the current scope
Token get_scope_associated_var(const Symbol_table scopes) {
	Token associated_var = NULL_TOKEN;
	for (int i = 0; i < scopes.scopes_count; i ++) {
		if (!compare_str_of_tokens(scopes.scopes[i].associated_var,  NULL_TOKEN)) {
			associated_var = scopes.scopes[i].associated_var;
		}
	}
	return associated_var;
}

// check if a statement is valid, if it is not, report it and halt
void check_statement(Symbol_table * variables, const Node_Statement stmt) {
	switch (stmt.statement_type) {
		case var_declaration_type: {
			// check that the expresion in the statement is valid
			Node_Expresion expresion = stmt.statement_value.var_declaration.value;
			is_expresion_valid(*variables, expresion);

			// check that when declaring a var there isnt another var with the same name
			Symbol variable = {
				.value=stmt.statement_value.var_declaration.var_name,
				.type=stmt.statement_value.var_declaration.type
			};
			if (is_var_in_var_list(*variables, variable.value)) {
				const Token previous_var = get_symbol_from_token(*variables, variable.value).value;
				errorf("Line:%d, column:%d.  Error: variable \"%t\" already declared in line:%d, column:%d.\n", variable.value.line_number, variable.value.column_number, variable.value, previous_var.line_number, previous_var.column_number);
			}
			else {
				append_var_to_var_list(variable, variables);
			}

			// check that the types of the declaration are valid with the ones of the expresion
			if (!compare_2_types(variable.type, get_type_of_expresion(*variables, expresion))) {
				errorf("Line:%d, column:%d.  Error: the type in variable declaration does not match the expression type.\n", variable.value.line_number, variable.value.column_number);
			}
			break;
		}
		case exit_node_type: {
			Node_Expresion expresion = stmt.statement_value.exit_node.exit_code;
			is_expresion_valid(*variables, expresion);
			break;
		}
		case print_type: {
			Node_Expresion expresion = stmt.statement_value.print.chr;
			is_expresion_valid(*variables, expresion);
			break;
		}
		case var_assignment_type: {
			Node_Var_assignment var_assgn = stmt.statement_value.var_assignment;
			// check that the source expresion is valid
			Node_Expresion expresion = var_assgn.value;
			is_expresion_valid(*variables, expresion);
			Node_Type expr_type = get_type_of_expresion(*variables, expresion);
			switch (var_assgn.destination.destination_type) {
				case assgn_dest_var_name_type:
					// check that when assigning to the var there is not another var with the same name
					Token variable = var_assgn.destination.destination_value.var_name;
					if (!is_var_in_var_list(*variables, variable)) {
						errorf("Line:%d, column:%d.  Error: can not assign to a variable that has not been declared before.\n", variable.line_number, variable.column_number);
					}
					// check that the types of the variable and the expression match
					Node_Type var_type = get_symbol_from_token(*variables, variable).type;
					if (!compare_2_types(var_type, expr_type)) {
						error("the type of the expression and the variable does not match");
					}
					break;

				case assgn_dest_deref_type:
					// check that the destination expresion is valid
					Node_Expresion dest_expresion = *var_assgn.destination.destination_value.deref_expresion;
					is_expresion_valid(*variables, dest_expresion);
					// check that the types of the destination and the expression match
					Node_Type dest_type = get_type_of_expresion(*variables, dest_expresion);
					if (dest_type.type_type != type_ptr_type) {
						error("destination in pointer assignment has to be a pointer");
					}
					if (!compare_2_types(*dest_type.type_value.type_ptr_value, expr_type)) {
						error("the type of the right expression and the left expression does not match");
					}
					break;
				
				case assgn_dest_subscript_type:
					// check that the array variable has been declared before
					Token array = var_assgn.destination.destination_value.array_name;
					if (!is_var_in_var_list(*variables, array)) {
						errorf("Line:%d, column:%d.  Error: variable has not been declared before.\n", array.line_number, array.column_number);
					}
					// check that the variable is actually an array
					Node_Type array_type = get_symbol_from_token(*variables, array).type;
					if (array_type.type_type != type_array_type) {
						errorf("Line:%d, column:%d.  Error: the variable is not an array.\n", array.line_number, array.column_number);						
					}
					// check that the index expression is valid
					Node_Expresion index_expr = *var_assgn.destination.destination_value.index_expr;
					is_expresion_valid(*variables, index_expr);
					// check that the types of the destination and the expression match
					if (!compare_2_types(*array_type.type_value.type_array_value->primitive_type, expr_type)) {
						error("the type of the right expression and the left expression does not match");
					}
					break;
			}
			break;
		}
		case scope_type: {
			create_scope(variables, NULL_TOKEN);
			for (int i = 0; i < stmt.statement_value.scope.statements_count; i++) {
				check_statement(variables, stmt.statement_value.scope.statements_node[i]);
			}
			remove_scope(variables);
			break;
		}
		case if_type: {
			Node_Expresion condition = stmt.statement_value.if_node.condition;
			is_expresion_valid(*variables, condition);
			Symbol_table * scope_vars = variables;
			create_scope(scope_vars, NULL_TOKEN);
			for (int i = 0; i < stmt.statement_value.if_node.scope.statements_count; i++) {
				check_statement(scope_vars, stmt.statement_value.if_node.scope.statements_node[i]);
			}
			remove_scope(scope_vars);
			if (stmt.statement_value.if_node.has_else_block) {
				create_scope(scope_vars, NULL_TOKEN);
				for (int i = 0; i < stmt.statement_value.if_node.else_block.statements_count; i++) {
					check_statement(scope_vars, stmt.statement_value.if_node.else_block.statements_node[i]);
				}
				remove_scope(scope_vars);
			}
			break;
		}
		case while_type: {
			Node_Expresion condition = stmt.statement_value.while_node.condition;
			is_expresion_valid(*variables, condition);
			create_scope(variables, NULL_TOKEN);
			for (int i = 0; i < stmt.statement_value.while_node.scope.statements_count; i++) {
				check_statement(variables, stmt.statement_value.while_node.scope.statements_node[i]);
			}
			remove_scope(variables);
			break;
		}
		case func_def_type: {
			Symbol func = {
				.value=stmt.statement_value.func_def.name,
				.type=stmt.statement_value.func_def.type
			};
			if (is_checking_func_def || variables->scopes_count > 1) {
				errorf("Line:%d, column:%d.  Error: can only declare a function in the global scope.\n", func.value.line_number, func.value.column_number);
			}
			if (is_var_in_var_list(*variables, func.value)) {
				const Token previous_var = get_symbol_from_token(*variables, func.value).value;
				errorf("Line:%d, column:%d.  Error: symbol with name \"%t\" already declared in line:%d, column:%d.\n", func.value.line_number, func.value.column_number, func.value, previous_var.line_number, previous_var.column_number);
			}
			else {
				append_var_to_var_list(func, variables);
			}
			create_scope(variables, func.value);
			append_var_to_var_list(func, variables);
			for (int i = 0; i < stmt.statement_value.func_def.type.type_value.type_func_value->args_count; i++) {
				Symbol arg = {
					.value=stmt.statement_value.func_def.type.type_value.type_func_value->args_name[i],
					.type=stmt.statement_value.func_def.type.type_value.type_func_value->args_type[i]
				};
				if (is_var_in_var_list(*variables, arg.value)) {
					const Token previous_sym = get_symbol_from_token(*variables, arg.value).value;
					errorf("Line:%d, column:%d.  Error: symbol with name \"%t\" already declared in line:%d, column:%d.\n", arg.value.line_number, arg.value.column_number, arg.value, previous_sym.line_number, previous_sym.column_number);
				}
				append_var_to_var_list(arg, variables);
			}
			is_checking_func_def = true;
			for (int i = 0; i < stmt.statement_value.func_def.scope.statements_count; i++) {
				check_statement(variables, stmt.statement_value.func_def.scope.statements_node[i]);
			}
			is_checking_func_def = false;
			remove_scope(variables);
			break;
		}
		case return_type: {
			Node_Return return_node = stmt.statement_value.return_node;
			if (!is_checking_func_def) {
				errorf("Line:%d, column:%d.  Error: can only return from inside a function.\n", return_node.return_token.line_number, return_node.return_token.column_number);
			}
			if (return_node.returns_value) {
				is_expresion_valid(*variables, return_node.ret_value);
			}
			Node_Type current_func_type = get_symbol_from_token(*variables, get_scope_associated_var(*variables)).type;
			bool types_match = true;
			if (current_func_type.type_value.type_func_value->returns_value != return_node.returns_value) {
				types_match = false;
			}
			if (types_match && return_node.returns_value) {
				types_match = compare_2_types(get_type_of_expresion(*variables, return_node.ret_value), current_func_type.type_value.type_func_value->return_type);
			}
			if (!types_match) {
				errorf("Line:%d, column:%d.  Error: the returned type does not match the expected type to be returned.\n", return_node.return_token.line_number, return_node.return_token.column_number);
			}
			break;
		}
		case expresion_stmt_type: {
			Node_Expresion expresion = stmt.statement_value.expresion_stmt;
			is_expresion_valid(*variables, expresion);
			break;
		}
	}
}

// checks if the program follows the grammar rules and the language specifications
bool is_valid_program(Node_Program program) {
	Symbol_table scopes;
	scopes.scopes_count = 0;
	scopes.scopes = malloc(scopes.scopes_count * sizeof(Symbols_scope));
	create_scope(&scopes, NULL_TOKEN); // create the first global scope
	// check each statement correctness
	for (int i = 0; i < program.statements_count; i++) {
		Node_Statement statement = program.statements_node[i];
		check_statement(&scopes, statement);
	}
	free_symbol_table(scopes);
	return true;
}

#endif