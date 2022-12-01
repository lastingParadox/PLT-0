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

instruction *parse(int code_flag, int table_flag, lexeme *list)
{
  printf("in instruction\n");

	tokens = list;
  
  printf("before table calloc\n");
  
	table = calloc(ARRAY_SIZE, sizeof(symbol));

  printf("before code calloc\n");

	code = calloc(ARRAY_SIZE, sizeof(instruction));

	// PROGRAM
	level = -1;
 
  printf("in block function\n");
	block_function();
  printf("leaving block function\n");
  
	// Checks for stopping errors
  printf("error after block = %d\n", error);
	if (error == -1)
		return NULL;

	// Check for Error 1
	if(tokens[token_index].type != period) {
		error = 1;
		print_parser_error(1, 0);
	}

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
    printf("returning code\n");
		return code;
	}
  printf("returning null\n");
	return NULL;
}

// BLOCK
void block_function() {
	level++;
  
  printf("in declarations function\n");
	declarations_function();
  printf("leaving declarations function\n");
	if (error == -1)
		return;
  
  printf("in statement function\n");
	statement_function();
  printf("leaving statement function\n");
	if (error == -1)
		return;

	mark();

	level--;
}

// DECLARATIONS
void declarations_function() {
	int variables = 0;

	while(tokens[token_index].type == keyword_const || tokens[token_index].type == keyword_var || tokens[token_index].type == keyword_procedure) {
		if (tokens[token_index].type == keyword_const) {
      printf("in const function %d\n", tokens[token_index].type);
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
		token_index++;
	}
	emit(INC, 0, variables + 3);
}

// CONST
void const_function() {
	table[table_index].kind = 1;
	table[table_index].level = level;
	table[table_index].address = 0;

	int mult_flag = 1;

	// ident
	token_index++;
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
	token_index++;
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
	token_index++;
	if (tokens[token_index].type == minus) {
		mult_flag = -1;
		token_index++;
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
	token_index++;
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
	table[table_index].kind = 2;
	table[table_index].level = level;
	table[table_index].value = 0;
	table[table_index].address = var_amount + 3;

	// ident
	token_index++;
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
	token_index++;
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
	table[table_index].kind = 3;
	table[table_index].level = level;
	table[table_index].value = 0;
	table[table_index].address = -1;

	// ident
	token_index++;
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
	token_index++;
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
  printf("statement in  is %d\n", tokens[token_index].type);
	// Declare variables outside of switch
	int l;
	int m;
	int symbol_index;
	int jump_index;
	int follow;
	int first_instruction;
	int save_flag;
  printf("Entering Statement %d\n",tokens[token_index].type);
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

			// :=
			token_index++;
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
         expression_function();
         
        
        
  			if (error == -1)
  				return;
  
  			emit(STO, l, m);
      return;
			break;
		case keyword_call:
			// ident
			token_index++;
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
			}
	
			emit(CAL, l, m);

			break;
		case keyword_begin:
      
			do {
				token_index++;
        
				statement_function();
        printf("statement out is %d\n", tokens[token_index].type);
				if (error == -1)
					return;

			} while(tokens[token_index].type == semicolon);
      
			// end
			// Check for Error 6-4 and Error 10

			// -- SOMETHING IS GOING WRONG FROM HERE --
			if (tokens[token_index].type != keyword_end) {
				follow = tokens[token_index].type;
        printf("follow = %d\n", follow);
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
          printf("follow = %d\n", follow);
					print_parser_error(10, 0);

					// If the token is not a '.', '}', or a ';', stopping error.
					if (follow != period && follow != right_curly_brace && follow != semicolon) {
						error = -1;
						return;
					}
					
					error = 1;
				}
			}
			//	-- TO HERE --
			break;
		case keyword_if:

			condition_function();
			if (error == -1)
				return;

			jump_index = code_index;

			emit(JPC, 0, 0);

			// then
			
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
      
      token_index++;
			statement_function();
			if (error == -1)
				return;

			code[jump_index].m = code_index;

			break;
		case keyword_while:

			first_instruction = code_index;

			condition_function();

			jump_index = code_index;

			emit(JPC, 0, 0);

			// do
			
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
      
      token_index++;
			statement_function();
			if (error == -1)
				return;

			emit(JMP, 0, first_instruction);

			code[jump_index].m = code_index;

			break;
		case keyword_read:

			// ident
			token_index++;

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
			}

			emit(RED, 0, 0);
			emit(STO, l, m);
			
			break;
		case keyword_write:

			expression_function();
      token_index-=1;
			if (error == -1)
				return;
			
			emit(WRT, 0, 0);
      printf("writing %d\n", tokens[token_index+1].type);
      return;
			break;
		case keyword_def:
			save_flag = 1;
			int symbol_index;

			// ident
			token_index++;
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
			}

			// {
			token_index++;
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

			block_function();
			if (error == -1)
				return;

			if (save_flag) {
				table[symbol_index].address = code_index;
			}

			if (code[code_index - 1].op != RTN) {
				emit(RTN, 0, 0);
			}

			code[jump_index].m = code_index;

			// Uncertain if I should do this because of the block_function inception, so...
			// }, maybe? If we keep on getting a 16 error, it's likely because of this
			// On second thought, my use of token_index++ could be problematic in other areas.
			// Dunno, haven't tested it.
			token_index++;
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

			if (level == 0)
				emit(HLT, 0, 0);
			else
				emit(RTN, 0, 0);

			break;
		default:
			return;
	}
	// Starting to doubt if I should do this
	// I mean, it makes sense to me
	token_index++;
}

// CONDITION
void condition_function() {
  printf("in condition\n");
	expression_function();
  printf("condition is %d\n",  tokens[token_index].type);
	if (error == -1) {
		return;
	}

	// Operator
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

	expression_function();
  printf("then is %d\n",  tokens[token_index].type);
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
  
  	term_function();
   
   
  	if (error == -1) {
  		return;
  	}
  
  	// + or -
    printf("expression in  %d\n", tokens[token_index].type);
    
  	while(tokens[token_index].type == plus || tokens[token_index].type == minus) {
  
  		term_function();
  		if (error == -1) {
  			return;
  		}
    
      
      printf("ADD / SUB\n");
  		if (tokens[token_index].type == plus)
  			emit(ADD, 0, 0);
  		else if (tokens[token_index].type == minus)
  			emit(SUB, 0, 0);
  		
  		//token_index++;
      
  	}
 
 printf("expression out %d\n", tokens[token_index].type);
}

// TERM
void term_function() {

	factor_function();
	if (error == -1) {
		return;
	}
  
	// * or /
	token_index++;
    printf("term in  %d\n", tokens[token_index].type);
		while(tokens[token_index].type == times || tokens[token_index].type == division) {
    
		factor_function();
		if (error == -1) {
			return;
		}

    printf("MUL / DIV\n");
		if (tokens[token_index].type == times)
			emit(MUL, 0, 0);
		else if(tokens[token_index].type == division)
			emit(DIV, 0, 0);
		
		token_index++;
    
	}
 printf("term out %d\n", tokens[token_index].type);
}

// FACTOR
void factor_function() {
	
	// ident
	token_index++;
  printf("ident is %d\n",tokens[token_index].type);

	// This looks disgusting, but I'm trying to rush this out at this point
	// Check for Error 20
	if (tokens[token_index].type != identifier) {
		if (tokens[token_index].type != number) {
			if (tokens[token_index].type != left_parenthesis) {
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
			else {
				
				// Expression
        printf("%d after left parenthesis\n", tokens[token_index].type);
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
			}
		}
		else {
			emit(LIT, 0, tokens[token_index].number_value);
      //token_index++;
      printf("after number is %d\n", tokens[token_index].type);
		}
	}
	else {
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