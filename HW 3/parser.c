#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"

lexeme *tokens;
int token_index = 0;
symbol *table;
int table_index = 0;
instruction *code;
int code_index = 0;

int error = 0;
int level;

void emit(int op, int l, int m);
void add_symbol(int kind, char name[], int value, int level, int address);
void mark();
int multiple_declaration_check(char name[]);
int find_symbol(char name[], int kind);

void print_parser_error(int error_code, int case_code);
void print_assembly_code();
void print_symbol_table();

// GRAMMAR
void block_function();
void declarations_function();
void const_function();
void var_function(int var_amount);
void proc_function();
void statement_function();
void condition_function();
void expression_function();
void term_function();
void factor_function();

// DEBUG
void token_increment();
void token_decrement();
char* token_type_string(int type);

instruction *parse(int code_flag, int table_flag, lexeme *list)
{

	tokens = list;
	table = calloc(ARRAY_SIZE, sizeof(symbol));
	code = calloc(ARRAY_SIZE, sizeof(instruction));

	// PROGRAM
    printf("%s\n", "PROGRAM");
	level = -1;
 
    printf("%s\n",token_type_string(tokens[token_index].type));
	block_function();
  
	// Checks for stopping errors
	if (error == -1)
		return NULL;

	// Check for Error 1
	if(tokens[token_index].type != period) {
		error = 1;
		print_parser_error(1, 0);
	}
    // pass over .
    token_increment();

	// Loop for CAL instructions
	for (int i = 0; i < code_index; i++) {

		if (code[i].op == CAL) {
			if (code[i].m == -1) {
				continue;
			}
			symbol procedure = table[code[i].m];

			// Check for Error 21
			if (procedure.address == -1) {
				error = 1;
				print_parser_error(21, 0);
			}
			// Otherwise, set the M field of the instruction to the procedure address.
			else {
				code[i].m = procedure.address;
			}
		}
	}

	// If HLT instruction is implicit, add HLT to code array.
	if (code[code_index - 1].op != HLT) {
		emit(HLT, 0, 0);
	}

	// PROGRAM END

	if (error == 0) {
		if (code_flag)
			print_assembly_code();
		if (table_flag)
			print_symbol_table();
		return code;
	}
	return NULL;
}

// BLOCK
void block_function() {
    printf("%s\n", "BLOCK");
	level++;
  
	declarations_function();
	if (error == -1)
		return;
    
    token_increment();

	statement_function();
	if (error == -1)
		return;

	mark();

	level--;
}

// DECLARATIONS
void declarations_function() {
    printf("%s\n", "DECLARATIONS");
	int variables = 0;

	while(tokens[token_index].type == keyword_const || tokens[token_index].type == keyword_var || tokens[token_index].type == keyword_procedure) {
		if (tokens[token_index].type == keyword_const) {
			const_function();
		}
		else if (tokens[token_index].type == keyword_var) {
			var_function(variables);
			variables++;
		}
		else if (tokens[token_index].type == keyword_procedure) {
			proc_function();
		}

		if (error == -1)
			return;

		table_index++;
		token_increment();
	}

    // Block increases token already
    token_decrement();

	emit(INC, 0, variables + 3);
}

// CONST
void const_function() {
    printf("%s\n", "CONST");
	table[table_index].kind = 1;
	table[table_index].level = level;
	table[table_index].address = 0;

	int mult_flag = 1;

	// ident
	token_increment();
	// Check for Error 2-1
	if (tokens[token_index].type != identifier) {
		print_parser_error(2, 1);

		// If the token is not an assignment token, stopping error.
		if(tokens[token_index].type != assignment_symbol) {
			error = -1;
			return;
		}
		error = 1;
		strcpy(table[table_index].name, "null");
	}
	else {
		strcpy(table[table_index].name, tokens[token_index].identifier_name);

		// Check for Error 3
		if (multiple_declaration_check(table[table_index].name) != -1) {
			error = 1;
			print_parser_error(3, 0);
		}
	}

	// :=
	token_increment();
	// Check for Error 4-1
	if (tokens[token_index].type != assignment_symbol) {
		print_parser_error(4, 1);

		// Since minus is optional, we check for a minus token.
		if (tokens[token_index].type == minus) {
			// If the token after the minus token is not a number, stopping error.
			if (tokens[token_index + 1].type != number) {
				error = -1;
				return;
			}
		}
		// If the token is not a number, stopping error.
		else if (tokens[token_index].type != number) {
			error = -1;
			return;
		}

		error = 1;
	}

	// Optional "-" and number
	token_increment();
	if (tokens[token_index].type == minus) {
		mult_flag = -1;
        // number
		token_increment();
	}

	// Checking for Error 5
	if (tokens[token_index].type != number) {
		print_parser_error(5, 0);

		// If the token is not a semicolon, stopping error.
		if (tokens[token_index].type != semicolon) {
			error = -1;
			return;
		}

		error = 1;
		table[table_index].value = 0;
	}
	else {
		table[table_index].value = mult_flag * tokens[token_index].number_value;
	}

	// ;
	token_increment();
	// Checking for Error 6-1
	if (tokens[token_index].type != semicolon) {
		print_parser_error(6, 1);

		int follow = tokens[token_index].type;

		// If the next token is not in the follow set, stopping error.
		if (follow != keyword_const && follow != keyword_var && follow != keyword_procedure && follow != identifier &&
			follow != keyword_call && follow != keyword_begin && follow != keyword_if && follow != keyword_while &&
			follow != keyword_read && follow != keyword_write && follow != keyword_def && follow != keyword_return &&
			follow != period && follow != right_curly_brace) {
			
			error = -1;
			return;
		}

		error = 1;
	}
}

// VAR
void var_function(int var_amount) {
    printf("%s\n", "VAR");
	table[table_index].kind = 2;
	table[table_index].level = level;
	table[table_index].value = 0;
	table[table_index].address = var_amount + 3;

	// ident
	token_increment();
	// Check for Error 2-2
	if (tokens[token_index].type != identifier) {
		print_parser_error(2, 2);

		// If the token is not a semicolon, stopping error.
		if(tokens[token_index].type != semicolon) {
			error = -1;
			return;
		}
		error = 1;
		strcpy(table[table_index].name, "null");
	}
	else {
		strcpy(table[table_index].name, tokens[token_index].identifier_name);

		// Check for Error 3
		if (multiple_declaration_check(table[table_index].name) != -1) {
			error = 1;
			print_parser_error(3, 0);
		}
	}

	// ;
	token_increment();
	// Checking for Error 6-2
	if (tokens[token_index].type != semicolon) {
		print_parser_error(6, 2);

		int follow = tokens[token_index].type;

		// If the next token is not in the follow set, stopping error.
		if (follow != keyword_const && follow != keyword_var && follow != keyword_procedure && follow != identifier &&
			follow != keyword_call && follow != keyword_begin && follow != keyword_if && follow != keyword_while &&
			follow != keyword_read && follow != keyword_write && follow != keyword_def && follow != keyword_return &&
			follow != period && follow != right_curly_brace) {
			
			error = -1;
			return;
		}

		error = 1;
	}
}

// PROCEDURE
void proc_function() {
    printf("%s\n", "PROC");
	table[table_index].kind = 3;
	table[table_index].level = level;
	table[table_index].value = 0;
	table[table_index].address = -1;

	// ident
	token_increment();
	// Check for Error 2-3
	if (tokens[token_index].type != identifier) {
		print_parser_error(2, 3);

		// If the token is not an semicolon token, stopping error.
		if(tokens[token_index].type != semicolon) {
			error = -1;
			return;
		}
		error = 1;
		strcpy(table[table_index].name, "null");
	}
	else {
		strcpy(table[table_index].name, tokens[token_index].identifier_name);

		// Check for Error 3
		if (multiple_declaration_check(table[table_index].name) != -1) {
			error = 1;
			print_parser_error(3, 0);
		}
	}

	// ;
	token_increment();
	// Checking for Error 6-3
	if (tokens[token_index].type != semicolon) {
		print_parser_error(6, 3);

		int follow = tokens[token_index].type;

		// If the next token is not in the follow set, stopping error.
		if (follow != keyword_const && follow != keyword_var && follow != keyword_procedure && follow != identifier &&
			follow != keyword_call && follow != keyword_begin && follow != keyword_if && follow != keyword_while &&
			follow != keyword_read && follow != keyword_write && follow != keyword_def && follow != keyword_return &&
			follow != period && follow != right_curly_brace) {
			
			error = -1;
			return;
		}

		error = 1;
	}
}

// STATEMENT
void statement_function() {
    printf("%s\n", "STATEMENT");
	// Declare variables outside of switch
	int l;
	int m;
	int symbol_index;
	int jump_index;
	int follow;
	int first_instruction;
	int save_flag;
	switch(tokens[token_index].type) {
		case identifier:
			
      		symbol_index = find_symbol(tokens[token_index].identifier_name, 2);

			// Check for Error 8-1 and Error 7
			if (symbol_index == -1) {
				error = 1;
				// Error 8-1
				if (find_symbol(tokens[token_index].identifier_name, 1) == -1 && find_symbol(tokens[token_index].identifier_name, 3) == -1)
					print_parser_error(8, 1);
				// Error 7
				else
					print_parser_error(7, 0);
				
				l = -1;
				m = -1;
			}
			else {
				l = level - table[symbol_index].level;
				m = table[symbol_index].address;
			}

			// ident -> :=
			token_increment();
			// Check for Error 4-2
			if (tokens[token_index].type != assignment_symbol) {
				print_parser_error(4, 2);
      
				// If the token is not an identifier, number, or '(', stopping error.
				if (tokens[token_index].type != identifier && tokens[token_index].type != number &&
					tokens[token_index].type != left_parenthesis) {
					error = -1;
					return;
				}

				error = 1;
			}
            
            // := -> expression
            token_increment();

         	expression_function();
        
  			if (error == -1)
  				return;
  
  			emit(STO, l, m);

			break;

		case keyword_call:

			// call -> ident
			token_increment();
			// Check for Error 2-4
			if (tokens[token_index].type != identifier) {
				print_parser_error(2, 4);

				// If the token is not a '.' or a '}', stopping error.
				if (tokens[token_index].type != period && tokens[token_index].type != right_curly_brace) {
					error = -1;
					return;
				}

				error = 1;
				l = -1;
				m = -1;
			}
			else {
				symbol_index = find_symbol(tokens[token_index].identifier_name, 3);

				// Check for Error 8-2 and Error 9
				if (symbol_index == -1) {
					error = 1;
					// Error 8-1
					if (find_symbol(tokens[token_index].identifier_name, 1) == -1 && find_symbol(tokens[token_index].identifier_name, 2) == -1)
						print_parser_error(8, 2);
					// Error 7
					else
						print_parser_error(9, 0);
					
					l = -1;
					m = -1;
				}
				else {
					l = level - table[symbol_index].level;
					m = symbol_index;
				}

                // pass over ident
                token_increment();
			}
	
			emit(CAL, l, m);

			break;
		case keyword_begin:
   
			do {
                // begin -> statement (first time) ; -> statement (after first time)
				token_increment();
        
				statement_function();
				if (error == -1)
					return;

			} while(tokens[token_index].type == semicolon);
      
      
			// end
			// Check for Error 6-4 and Error 10

			if (tokens[token_index].type != keyword_end) {
				follow = tokens[token_index].type;
				// Error 6-4
				if (follow == identifier || follow == keyword_call || follow == keyword_begin || follow == keyword_if ||
					follow == keyword_while || follow == keyword_read || follow == keyword_write || follow == keyword_def ||
					follow == keyword_return) {
					print_parser_error(6, 4);
					error = -1;
					return;
				}
				// Error 10
				else {
					print_parser_error(10, 0);

					// If the token is not a '.', '}', or a ';', stopping error.
					if (follow != period && follow != right_curly_brace && follow != semicolon) {
						error = -1;
						return;
					}
					
					error = 1;
				}
			}

            // pass over end
            token_increment();

			break;
		case keyword_if:

            // if -> condition
            token_increment();

			condition_function();
			if (error == -1)
				return;

			jump_index = code_index;

			emit(JPC, 0, 0);
			
			// Check for Error 11
			if (tokens[token_index].type != keyword_then) {
				print_parser_error(11, 0);

				follow = tokens[token_index].type;

				// If the token is not in the follow set, stopping error.
				if (follow != period && follow != right_curly_brace && follow != semicolon && follow != keyword_end &&
					follow != identifier && follow != keyword_call && follow != keyword_begin && follow != keyword_if &&
					follow != keyword_while && follow != keyword_read && follow != keyword_write && follow != keyword_def &&
					follow != keyword_return) {
						error = -1;
						return;
				}

				error = 1;
			}
      
            // then -> statement
            token_increment();

			statement_function();
			if (error == -1)
				return;

			code[jump_index].m = code_index;

			break;
		case keyword_while:

			first_instruction = code_index;

            // while -> condition
            token_increment();

			condition_function();

			jump_index = code_index;

			emit(JPC, 0, 0);

			// Check for Error 12
			if (tokens[token_index].type != keyword_do) {
				print_parser_error(12, 0);

				follow = tokens[token_index].type;

				// If the token is not in the follow set, stopping error.
				if (follow != period && follow != right_curly_brace && follow != semicolon && follow != keyword_end &&
					follow != identifier && follow != keyword_call && follow != keyword_begin && follow != keyword_if &&
					follow != keyword_while && follow != keyword_read && follow != keyword_write && follow != keyword_def &&
					follow != keyword_return) {
						error = -1;
						return;
				}

				error = 1;
			}
      
            // do -> statement
            token_increment();

			statement_function();
			if (error == -1)
				return;

			emit(JMP, 0, first_instruction);

			code[jump_index].m = code_index;

			break;
		case keyword_read:

			// read -> ident
			token_increment();

			// Check for 2-5
			if (tokens[token_index].type != identifier) {
				print_parser_error(2, 5);

				// If the token is not '.', '}', ';' or 'end', stoping error.
				if (tokens[token_index].type != period && tokens[token_index].type != right_curly_brace && 
					tokens[token_index].type != semicolon && tokens[token_index].type != keyword_end) {
						error = -1;
						return;
				}
				error = 1;
				l = -1;
				m = -1;
			}
			else {
				symbol_index = find_symbol(tokens[token_index].identifier_name, 2);

				// Check for Error 8-3 and 13
				if (symbol_index == -1) {
					error = 1;
					// Error 8-3
					if (find_symbol(tokens[token_index].identifier_name, 1) == -1 && find_symbol(tokens[token_index].identifier_name, 3) == -1)
						print_parser_error(8, 3);
					// Error 13
					else
						print_parser_error(13, 0);
					
					l = -1;
					m = -1;
				}
				else {
					l = level - table[symbol_index].level;
					m = table[symbol_index].address;
				}

                // pass over ident
                token_increment();
			}

			emit(RED, 0, 0);
			emit(STO, l, m);
			
			break;
		case keyword_write:

            // write -> expression
            token_increment();

			expression_function();
			if (error == -1)
				return;
			
			emit(WRT, 0, 0);
			break;
		case keyword_def:
			save_flag = 1;
			int symbol_index;

			// def -> ident
			token_increment();
			// Check for 2-6
			if (tokens[token_index].type != identifier) {
				print_parser_error(2, 6);

				// If the token is not '{', stopping error.
				if (tokens[token_index].type != left_curly_brace) {
					error = -1;
					return;
				}

				error = 1;
				save_flag = 0;
			}
			else {
				symbol_index = find_symbol(tokens[token_index].identifier_name, 3);

				// Check for Error 8-4 and 14
				if (symbol_index == -1) {
					error = 1;
					// Error 8-4
					if (find_symbol(tokens[token_index].identifier_name, 1) == -1 && find_symbol(tokens[token_index].identifier_name, 2) == -1)
						print_parser_error(8, 4);
					// Error 14
					else
						print_parser_error(14, 0);
				
				}
				// Check for Error 22
				if (table[symbol_index].level != level) {
					print_parser_error(22, 0);
					error = 1;
				}
				// Check for Error 23
				else if (table[symbol_index].address != -1) {
					print_parser_error(23, 0);
					error = 1;
				}

                // ident -> {
                token_increment();
			}

			// Check for Error 15
			if (tokens[token_index].type != left_curly_brace) {
				print_parser_error(15, 0);

				int follow = tokens[token_index].type;

				// If the token is not in the follow set, stopping error.
				if (follow != keyword_const && follow != keyword_var && follow != keyword_procedure && follow != identifier &&
					follow != keyword_call && follow != keyword_begin && follow != keyword_if && follow != keyword_while &&
					follow != keyword_read && follow != keyword_write && follow != keyword_def && follow != keyword_return &&
					follow != right_curly_brace) {
						error = -1;
						return;
				}

				error = 1;
			}

			int jump_index = code_index;

			emit(JMP, 0, 0);

            // { -> block
            token_increment();

			block_function();
      
      		if(tokens[token_index].type == right_curly_brace)
        		token_index--;
        
			if (error == -1)
				return;

			if (save_flag) {
				table[symbol_index].address = code_index;
			}

			if (code[code_index - 1].op != RTN) {
				emit(RTN, 0, 0);
			}

			code[jump_index].m = code_index;

			// Check for Error 16
			if (tokens[token_index].type != right_curly_brace) {
				print_parser_error(16, 0);

				// If the token is not '.', ';' or 'end', stopping error.
				if (tokens[token_index].type != period && tokens[token_index].type != semicolon && tokens[token_index].type != keyword_end) {
					error = -1;
					return;
				}

				error = 1;
			}

			break;
		case keyword_return:
            // pass over return
            token_increment();

			if (level == 0)
				emit(HLT, 0, 0);
			else
				emit(RTN, 0, 0);
        token_index++;
      return;
			break;
		default:
            return;
	}
}

// CONDITION
void condition_function() {
    printf("%s\n", "CONDITION");

	expression_function();

	if (error == -1) {
		return;
	}

	int type = tokens[token_index].type;

	// Check for Error 17
	if (type != equal_to && type != not_equal_to && type != less_than && type != less_than_or_equal_to &&
		type != greater_than && type != greater_than_or_equal_to) {
		print_parser_error(17, 0);

		// If the token is not an identifer, a number, or '(', stopping error.
		if (type != identifier && type != number && type != left_parenthesis) {
			error = -1;
			return;
		}

		error = 1;
	}

    // operator -> expression
    token_increment();

	expression_function();
	if (error == -1) {
		return;
	}

	switch(type) {
			case equal_to:
				emit(EQL, 0, 0);
				break;
			case not_equal_to:
				emit(NEQ, 0, 0);
				break;
			case less_than:
				emit(LSS, 0, 0);
				break;
			case less_than_or_equal_to:
				emit(LEQ, 0, 0);
				break;
			case greater_than:
				emit(GTR, 0, 0);
				break;
			case greater_than_or_equal_to:
				emit(GEQ, 0, 0);
				break;
			default:
				emit(OPR, 0 , -1);
	}
}

// EXPRESSION
void expression_function() {
    printf("%s\n", "EXPRESSION");
  	term_function();
   
  	if (error == -1) {
  		return;
  	}
    
  	while(tokens[token_index].type == plus || tokens[token_index].type == minus) {
  
        int type = tokens[token_index].type;

        // term
        token_increment();

  		term_function();
  		if (error == -1) {
  			return;
  		}
      
  		if (type == plus)
  			emit(ADD, 0, 0);
  		else if (type == minus)
  			emit(SUB, 0, 0);
      
  	}
}

// TERM
void term_function() {
    printf("%s\n", "TERM");

	factor_function();
	if (error == -1) {
		return;
	}
  
	while(tokens[token_index].type == times || tokens[token_index].type == division) {
    
        int type = tokens[token_index].type;

        // factor
        token_increment();

		factor_function();
		if (error == -1) {
			return;
		}

		if (type == times)
			emit(MUL, 0, 0);
		else if(type == division)
			emit(DIV, 0, 0);

	}
}

// FACTOR
void factor_function() {
	printf("%s\n", "FACTOR");
	// ident, number, ( EXPRESSION )

    if (tokens[token_index].type == identifier) {
        int constant_index = find_symbol(tokens[token_index].identifier_name, 1);
		int var_index = find_symbol(tokens[token_index].identifier_name, 2);

		// Check for Error 8-5 and 18
		if (constant_index == -1 && var_index == -1) {
			error = 1;

			// Error 8-5
			if (find_symbol(tokens[token_index].identifier_name, 3) == -1) {
				print_parser_error(8, 5);
			}
			// Error 18
			else {
				print_parser_error(18, 0);
			}
		}
		else if (constant_index != -1 && var_index == -1) {
			emit(LIT, 0, table[constant_index].address);
		}
		else if (constant_index == -1 && var_index != -1) {
			emit(LOD, level - table[var_index].level, table[var_index].address);
		}
		else {
			if (table[constant_index].level > table[var_index].level) {
				emit(LIT, 0, table[constant_index].address);
			}
			else {
				emit(LOD, level - table[var_index].level, table[var_index].address);
			}
		}

        // pass over ident
        token_increment();
    }

    else if (tokens[token_index].type == number) {
        emit(LIT, 0, tokens[token_index].number_value);

        // pass over number
        token_increment();
    }

    else if (tokens[token_index].type == left_parenthesis) {
        
        // ( -> expression
        token_increment();

        expression_function();
        if (error == -1)
            return;
        
        // Check for Error 19
        if (tokens[token_index].type != right_parenthesis) {
            print_parser_error(19, 0);

            int follow = tokens[token_index].type;

            // If the token is not in the follow set, stopping error.
            if (follow != plus && follow != division && follow != plus && follow != minus && follow != period && follow != right_curly_brace &&
                follow != semicolon && follow != keyword_end && follow != equal_to && follow != not_equal_to && follow != less_than &&
                follow != less_than_or_equal_to && follow != greater_than && follow != greater_than_or_equal_to && follow != keyword_then &&
                follow != keyword_do) {
                    error = -1;
                    return;
                }
            
            error = 1;
        }

        // Pass over '}'
        token_increment();

    }
    // Error 20
    else {
        print_parser_error(20, 0);
				
		int follow = tokens[token_index].type;

		// If the token is not in the follow set, stopping error.
		if (follow != plus && follow != division && follow != plus && follow != minus && follow != period && follow != right_curly_brace &&
			follow != semicolon && follow != keyword_end && follow != equal_to && follow != not_equal_to && follow != less_than &&
			follow != less_than_or_equal_to && follow != greater_than && follow != greater_than_or_equal_to && follow != keyword_then &&
			follow != keyword_do) {
			error = -1;        
			return;
		}
				
		error = 1;
    }
}

// Debug Function
void token_increment() {
    token_index++;
    printf("%s Increment\n", token_type_string(tokens[token_index].type));
}

void token_decrement() {
    token_index--;
    printf("%s Decrement\n", token_type_string(tokens[token_index].type));
}


char* token_type_string(int type) {
    switch (type) {
        case identifier:
            return "ident";
        case number:
            return "number";
        case keyword_const:
            return "const";
        case keyword_var:
            return "var";
        case keyword_procedure:
            return "procedure";
        case keyword_call:
            return "call";
        case keyword_begin:
            return "begin";
        case keyword_end:
            return "end";
        case keyword_if:
            return "if";
        case keyword_then:
            return "then";
        case keyword_while:
            return "while";
        case keyword_do:
            return "do";
        case keyword_read:
            return "read";
        case keyword_write:
            return "write";
        case keyword_return:
            return "return";
        case keyword_def:
            return "def";
        case period:
            return ".";
        case assignment_symbol:
            return ":=";
        case minus:
            return "-";
        case semicolon:
            return ";";
        case left_curly_brace:
            return "{";
        case right_curly_brace:
            return "}";
        case equal_to:
            return "==";
        case not_equal_to:
            return "!=";
        case less_than:
            return "<";
        case less_than_or_equal_to:
            return "<=";
        case greater_than:
            return ">";
        case greater_than_or_equal_to:
            return ">=";
        case plus:
            return "+";
        case times:
            return "*";
        case division:
            return "/";
        case left_parenthesis:
            return "(";
        case right_parenthesis:
            return ")";
        default:
            return "missing";
    }
}

// Provided functions
void emit(int op, int l, int m)
{
	code[code_index].op = op;
	code[code_index].l = l;
	code[code_index].m = m;
	code_index++;
}

void add_symbol(int kind, char name[], int value, int level, int address)
{
	table[table_index].kind = kind;
	strcpy(table[table_index].name, name);
	table[table_index].value = value;
	table[table_index].level = level;
	table[table_index].address = address;
	table[table_index].mark = 0;
	table_index++;
}

void mark()
{
	int i;
	for (i = table_index - 1; i >= 0; i--)
	{
		if (table[i].mark == 1)
			continue;
		if (table[i].level < level)
			return;
		table[i].mark = 1;
	}
}

int multiple_declaration_check(char name[])
{
	int i;
	for (i = 0; i < table_index; i++)
		if (table[i].mark == 0 && table[i].level == level && strcmp(name, table[i].name) == 0)
			return i;
	return -1;
}

int find_symbol(char name[], int kind)
{
	int i;
	int max_idx = -1;
	int max_lvl = -1;
	for (i = 0; i < table_index; i++)
	{
		if (table[i].mark == 0 && table[i].kind == kind && strcmp(name, table[i].name) == 0)
		{
			if (max_idx == -1 || table[i].level > max_lvl)
			{
				max_idx = i;
				max_lvl = table[i].level;
			}
		}
	}
	return max_idx;
}

void print_parser_error(int error_code, int case_code)
{
	switch (error_code)
	{
		case 1 :
			printf("Parser Error 1: missing . \n");
			break;
		case 2 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 2: missing identifier after keyword const\n");
					break;
				case 2 :
					printf("Parser Error 2: missing identifier after keyword var\n");
					break;
				case 3 :
					printf("Parser Error 2: missing identifier after keyword procedure\n");
					break;
				case 4 :
					printf("Parser Error 2: missing identifier after keyword call\n");
					break;
				case 5 :
					printf("Parser Error 2: missing identifier after keyword read\n");
					break;
				case 6 :
					printf("Parser Error 2: missing identifier after keyword def\n");
					break;
				default :
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 3 :
			printf("Parser Error 3: identifier is declared multiple times by a procedure\n");
			break;
		case 4 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 4: missing := in constant declaration\n");
					break;
				case 2 :
					printf("Parser Error 4: missing := in assignment statement\n");
					break;
				default :				
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 5 :
			printf("Parser Error 5: missing number in constant declaration\n");
			break;
		case 6 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 6: missing ; after constant declaration\n");
					break;
				case 2 :
					printf("Parser Error 6: missing ; after variable declaration\n");
					break;
				case 3 :
					printf("Parser Error 6: missing ; after procedure declaration\n");
					break;
				case 4 :
					printf("Parser Error 6: missing ; after statement in begin-end\n");
					break;
				default :				
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 7 :
			printf("Parser Error 7: procedures and constants cannot be assigned to\n");
			break;
		case 8 :
			switch (case_code)
			{
				case 1 :
					printf("Parser Error 8: undeclared identifier used in assignment statement\n");
					break;
				case 2 :
					printf("Parser Error 8: undeclared identifier used in call statement\n");
					break;
				case 3 :
					printf("Parser Error 8: undeclared identifier used in read statement\n");
					break;
				case 4 :
					printf("Parser Error 8: undeclared identifier used in define statement\n");
					break;
				case 5 :
					printf("Parser Error 8: undeclared identifier used in arithmetic expression\n");
					break;
				default :				
					printf("Implementation Error: unrecognized error code\n");
			}
			break;
		case 9 :
			printf("Parser Error 9: variables and constants cannot be called\n");
			break;
		case 10 :
			printf("Parser Error 10: begin must be followed by end\n");
			break;
		case 11 :
			printf("Parser Error 11: if must be followed by then\n");
			break;
		case 12 :
			printf("Parser Error 12: while must be followed by do\n");
			break;
		case 13 :
			printf("Parser Error 13: procedures and constants cannot be read\n");
			break;
		case 14 :
			printf("Parser Error 14: variables and constants cannot be defined\n");
			break;
		case 15 :
			printf("Parser Error 15: missing {\n");
			break;
		case 16 :
			printf("Parser Error 16: { must be followed by }\n");
			break;
		case 17 :
			printf("Parser Error 17: missing relational operator\n");
			break;
		case 18 :
			printf("Parser Error 18: procedures cannot be used in arithmetic\n");
			break;
		case 19 :
			printf("Parser Error 19: ( must be followed by )\n");
			break;
		case 20 :
			printf("Parser Error 20: invalid expression\n");
			break;
		case 21 :
			printf("Parser Error 21: procedure being called has not been defined\n");
			break;
		case 22 :
			printf("Parser Error 22: procedures can only be defined within the procedure that declares them\n");
			break;
		case 23 :
			printf("Parser Error 23: procedures cannot be defined multiple times\n");
			break;
		default:
			printf("Implementation Error: unrecognized error code\n");

	}
}

void print_assembly_code()
{
	int i;
	printf("Assembly Code:\n");
	printf("Line\tOP Code\tOP Name\tL\tM\n");
	for (i = 0; i < code_index; i++)
	{
		printf("%d\t%d\t", i, code[i].op);
		switch(code[i].op)
		{
			case LIT :
				printf("LIT\t");
				break;
			case OPR :
				switch (code[i].m)
				{
					case ADD :
						printf("ADD\t");
						break;
					case SUB :
						printf("SUB\t");
						break;
					case MUL :
						printf("MUL\t");
						break;
					case DIV :
						printf("DIV\t");
						break;
					case EQL :
						printf("EQL\t");
						break;
					case NEQ :
						printf("NEQ\t");
						break;
					case LSS :
						printf("LSS\t");
						break;
					case LEQ :
						printf("LEQ\t");
						break;
					case GTR :
						printf("GTR\t");
						break;
					case GEQ :
						printf("GEQ\t");
						break;
					default :
						printf("err\t");
						break;
				}
				break;
			case LOD :
				printf("LOD\t");
				break;
			case STO :
				printf("STO\t");
				break;
			case CAL :
				printf("CAL\t");
				break;
			case RTN :
				printf("RTN\t");
				break;
			case INC :
				printf("INC\t");
				break;
			case JMP :
				printf("JMP\t");
				break;
			case JPC :
				printf("JPC\t");
				break;
			case SYS :
				switch (code[i].m)
				{
					case WRT :
						printf("WRT\t");
						break;
					case RED :
						printf("RED\t");
						break;
					case HLT :
						printf("HLT\t");
						break;
					default :
						printf("err\t");
						break;
				}
				break;
			default :
				printf("err\t");
				break;
		}
		printf("%d\t%d\n", code[i].l, code[i].m);
	}
	printf("\n");
}

void print_symbol_table()
{
	int i;
	printf("Symbol Table:\n");
	printf("Kind | Name        | Value | Level | Address | Mark\n");
	printf("---------------------------------------------------\n");
	for (i = 0; i < table_index; i++)
		printf("%4d | %11s | %5d | %5d | %5d | %5d\n", table[i].kind, table[i].name, table[i].value, table[i].level, table[i].address, table[i].mark); 
	printf("\n");
}
