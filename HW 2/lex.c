/* 
 *  Written by:
 *    Samuel Georgis
 *    Evan Goldsmith
 *    Sean Jackson
 *    Zander Preston
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "compiler.h"

#define MAX_IDENT_LENGTH 11
#define MAX_NUM_LENGTH 5
#define ERR_IDENT_LENGTH 1
#define ERR_NUM_LENGTH 2
#define ERR_INVALID_IDENT 3
#define ERR_INVALID_SYMBOL 4
#define ERR_INVALID_IDENT_NAME 5

int keyword_check(char buffer[]);
void print_lexeme_list(lexeme *list, int list_end);

lexeme *lex_analyze(int list_flag, char *input)
{
	lexeme *list = (lexeme*) calloc(ARRAY_SIZE, sizeof(lexeme));
	int input_index = 0;
	int lex_index = 0;
	int buffer_index = 0;
	int error_flag = 0;

	char *buffer = (char*) calloc(MAX_IDENT_LENGTH + 1, sizeof(char));

	while (input[input_index] != '\0')
	{	
		int value = input[input_index];

		// Clearing buffer and its index
		strcpy(buffer, "");
		buffer_index = 0;

		if (iscntrl(value) || isspace(value))
		{
			input_index++;
			continue;
		}

		else if (isdigit(value))
		{
			while(isdigit(input[input_index]) && buffer_index != MAX_NUM_LENGTH)
			{
				buffer[buffer_index] = input[input_index];
				input_index++;
				buffer_index++;
			}

			input_index--;
			buffer[buffer_index] = '\0';

			if (isalnum(input[input_index + 1]))
			{
				if(isdigit(input[input_index + 1]))
				{
					list[lex_index].type = -1;
					list[lex_index].error_type = ERR_NUM_LENGTH;
				}

				else if (isalpha(input[input_index + 1]))
				{
					list[lex_index].type = -1;
					list[lex_index].error_type = ERR_INVALID_IDENT;
				}

				while(isalnum(input[input_index]))
				{
					input_index++;
				}

				// Must decrement index here as the loop will increment it after we exit
				input_index--;
			}

			else
			{
				char *digitString = (char*)malloc(sizeof(char) * buffer_index + 1);
				strcpy(digitString, buffer);

				list[lex_index].type = number;
				list[lex_index].number_value = atoi(digitString);

				free(digitString);
			}
		}
	
		else if (isalpha(value))
		{
			while(isalnum(input[input_index]) && buffer_index != MAX_IDENT_LENGTH - 1)
			{
				buffer[buffer_index] = input[input_index];
				input_index++;
				buffer_index++;
			}

			// Undo index change when loop breaks
			input_index--;

			buffer[buffer_index] = '\0';

			// Checks if next char is alnum
			if (isalnum(input[input_index + 1]))
			{

				list[lex_index].type = -1;
				list[lex_index].error_type = ERR_IDENT_LENGTH;

				// This loop will change the index until it reaches a new line
				while(isalnum(input[input_index]))
					input_index++;

				// Must decrement index here as the loop will increment it after
				input_index--;
			}

			else
			{
				int keyword = keyword_check(buffer);

				if (keyword == identifier)
				{
					list[lex_index].type = identifier;
					strcpy(list[lex_index].identifier_name, buffer);
				}
				else if (keyword != -1)
				{
					list[lex_index].type = keyword;
				}
				else
				{
					list[lex_index].type = -1;
					list[lex_index].error_type = ERR_INVALID_IDENT_NAME;
					error_flag = 1;
				}
			}
		}
		else
		{
			switch (value)
			{
				case '.':
					list[lex_index].type = period;
					break;
				case '-':
					list[lex_index].type = minus;
					break;
				case ';':
					list[lex_index].type = semicolon;
					break;
				case '{':
					list[lex_index].type = left_curly_brace;
					break;
				case '}':
					list[lex_index].type = right_curly_brace;
					break;
				case '+':
					list[lex_index].type = plus;
					break;
				case '*':
					list[lex_index].type = times;
					break;
				case '/':
					list[lex_index].type = division;
					break;
				case '(':
					list[lex_index].type = left_parenthesis;
					break;
				case ')':
					list[lex_index].type = right_parenthesis;
					break;
				case ':':
					if (input[input_index + 1] == '=')
					{
						list[lex_index].type = assignment_symbol;
						input_index++;

					}
					else
					{
						//error handle; ERR_INVALID_SYMBOL
						list[lex_index].type = -1;
						list[lex_index].error_type = ERR_INVALID_SYMBOL;
						error_flag = 1;
					}
					break;
				case '=':
					if (input[input_index + 1] == '=')
					{
						list[lex_index].type = equal_to;
						input_index++;

					}
					else
					{
						//error handle; ERR_INVALID_SYMBOL
						list[lex_index].type = -1;
						list[lex_index].error_type = ERR_INVALID_SYMBOL;
						error_flag = 1;
					}
					break;
				case '<':
					if (input[input_index + 1] == '=')
					{
						list[lex_index].type = less_than_or_equal_to;
						input_index++;

					}
					else
						list[lex_index].type = less_than;
					break;
				case '>':
					if (input[input_index + 1] == '=')
					{
						list[lex_index].type = greater_than_or_equal_to;
						input_index++;
					}
					else 
						list[lex_index].type = greater_than;
					break;
				case '!':
					if (input[input_index + 1] == '=')
					{
						list[lex_index].type = not_equal_to;
						input_index++;
					}
					else
					{
						// error handle; ERR_INVALID_SYMBOL
						list[lex_index].type = -1;
						list[lex_index].error_type = ERR_INVALID_SYMBOL;
						error_flag = 1;
					}
					break;
				case '?':
					// skip comments code here
					while (input[input_index + 1] != '\n' && input[input_index + 1] != '\0')
					{
						input_index++;
					}

					// Skips newline char
					if (input[input_index + 1] == '\n')
						input_index++;

					// Removes this index from lexical analyzer since it is a comment
					lex_index--;
					break;
				default:
					list[lex_index].error_type = ERR_INVALID_SYMBOL;
					list[lex_index].type = -1;
					error_flag = 1;
			}
		}
		input_index++;
		lex_index++;
	}

	// Conditionals return NULL or list depending on errors
	if (list_flag && !error_flag)
	{
		print_lexeme_list(list, lex_index);
		return list;
	}

	else if (list_flag && error_flag)
	{
		print_lexeme_list(list, lex_index);
		return NULL;
	}

	else if (!list_flag && !error_flag)
		return list;
	else
		return NULL;
}

int keyword_check(char buffer[])
{
	if (strcmp(buffer, "const") == 0)
		return keyword_const;
	else if (strcmp(buffer, "var") == 0)
		return keyword_var;
	else if (strcmp(buffer, "procedure") == 0)
		return keyword_procedure;
	else if (strcmp(buffer, "call") == 0)
		return keyword_call;
	else if (strcmp(buffer, "begin") == 0)
		return keyword_begin;
	else if (strcmp(buffer, "end") == 0)
		return keyword_end;
	else if (strcmp(buffer, "if") == 0)
		return keyword_if;
	else if (strcmp(buffer, "then") == 0)
		return keyword_then;
	else if (strcmp(buffer, "while") == 0)
		return keyword_while;
	else if (strcmp(buffer, "do") == 0)
		return keyword_do;
	else if (strcmp(buffer, "read") == 0)
		return keyword_read;
	else if (strcmp(buffer, "write") == 0)
		return keyword_write;
	else if (strcmp(buffer, "def") == 0)
		return keyword_def;
	else if (strcmp(buffer, "return") == 0)
		return keyword_return;
	else if (strcmp(buffer, "main") == 0)
		return -1;
	else if (strcmp(buffer, "null") == 0)
		return -1;
	else
		return identifier;
}

void print_lexeme_list(lexeme *list, int list_end)
{
	int i;
	printf("Lexeme List: \n");
	printf("lexeme\t\ttoken type\n");
	for (i = 0; i < list_end; i++)
	{
		// not an error
		if (list[i].type != -1)
		{
			switch (list[i].type)
			{
				case identifier :
					printf("%11s\t%d\n", list[i].identifier_name, identifier);
					break;
				case number :
					printf("%11d\t%d\n", list[i].number_value, number);
					break;
				case keyword_const :
					printf("%11s\t%d\n", "const", keyword_const);
					break;
				case keyword_var :
					printf("%11s\t%d\n", "var", keyword_var);
					break;
				case keyword_procedure :
					printf("%11s\t%d\n", "procedure", keyword_procedure);
					break;
				case keyword_call :
					printf("%11s\t%d\n", "call", keyword_call);
					break;
				case keyword_begin :
					printf("%11s\t%d\n", "begin", keyword_begin);
					break;
				case keyword_end :
					printf("%11s\t%d\n", "end", keyword_end);
					break;
				case keyword_if :
					printf("%11s\t%d\n", "if", keyword_if);
					break;
				case keyword_then :
					printf("%11s\t%d\n", "then", keyword_then);
					break;
				case keyword_while :
					printf("%11s\t%d\n", "while", keyword_while);
					break;
				case keyword_do :
					printf("%11s\t%d\n", "do", keyword_do);
					break;
				case keyword_read :
					printf("%11s\t%d\n", "read", keyword_read);
					break;
				case keyword_write :
					printf("%11s\t%d\n", "write", keyword_write);
					break;
				case keyword_def :
					printf("%11s\t%d\n", "def", keyword_def);
					break;
				case keyword_return :
					printf("%11s\t%d\n", "return", keyword_return);
					break;
				case period :
					printf("%11s\t%d\n", ".", period);
					break;
				case assignment_symbol :
					printf("%11s\t%d\n", ":=", assignment_symbol);
					break;
				case minus :
					printf("%11s\t%d\n", "-", minus);
					break;
				case semicolon :
					printf("%11s\t%d\n", ";", semicolon);
					break;
				case left_curly_brace :
					printf("%11s\t%d\n", "{", left_curly_brace);
					break;
				case right_curly_brace :
					printf("%11s\t%d\n", "}", right_curly_brace);
					break;
				case equal_to :
					printf("%11s\t%d\n", "==", equal_to);
					break;
				case not_equal_to :
					printf("%11s\t%d\n", "!=", not_equal_to);
					break;
				case less_than :
					printf("%11s\t%d\n", "<", less_than);
					break;
				case less_than_or_equal_to :
					printf("%11s\t%d\n", "<=", less_than_or_equal_to);
					break;
				case greater_than :
					printf("%11s\t%d\n", ">", greater_than);
					break;
				case greater_than_or_equal_to :
					printf("%11s\t%d\n", ">=", greater_than_or_equal_to);
					break;
				case plus :
					printf("%11s\t%d\n", "+", plus);
					break;
				case times :
					printf("%11s\t%d\n", "*", times);
					break;
				case division :
					printf("%11s\t%d\n", "/", division);
					break;
				case left_parenthesis :
					printf("%11s\t%d\n", "(", left_parenthesis);
					break;
				case right_parenthesis :
					printf("%11s\t%d\n", ")", right_parenthesis);
					break;
				default :
					printf("Implementation Error: unrecognized token type\n");
					break;
			}
		}
		// errors
		else
		{
			switch (list[i].error_type)
			{
				case ERR_IDENT_LENGTH :
					printf("Lexical Analyzer Error: maximum identifier length is 11\n");
					break;
				case ERR_NUM_LENGTH :
					printf("Lexical Analyzer Error: maximum number length is 5\n");
					break;
				case ERR_INVALID_IDENT :
					printf("Lexical Analyzer Error: identifiers cannot begin with digits\n");
					break;
				case ERR_INVALID_SYMBOL :
					printf("Lexical Analyzer Error: invalid symol\n");
					break;
				case ERR_INVALID_IDENT_NAME :
					printf("Lexical Analyzer Error: identifiers cannot be named 'null' or 'main'\n");
					break;
				default :
					printf("Implementation Error: unrecognized error type\n");
					break;
			}
		}
	}
	printf("\n");
}
