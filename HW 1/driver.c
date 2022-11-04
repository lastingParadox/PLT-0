#include <stdlib.h>
#include <stdio.h>
#include "compiler.h"

int main(int argc, char *argv[])
{
	// flags for printing output
	int list_flag = 0, code_flag = 0, table_flag = 0, trace_flag = 0;
	
	FILE *ifp;
	char *input = calloc(ARRAY_SIZE, sizeof(char));
	char buffer;
	int i;
	
	lexeme *list;
	instruction *code;
	
	// read in the input file
	if (argc < 2)
	{
		printf("Error : please include the file name\n");
		return 0;
	}
	
	ifp = fopen(argv[1], "r");
	i = 0;
	while (fscanf(ifp, "%c", &buffer) != EOF)
		input[i++] = buffer;
	input[i] = '\0';
	fclose(ifp);
	
	// check what should be printed
	for (i = 2; i < argc; i++)
	{
		if (argv[i][1] == 'l')
			list_flag = 1;
		if (argv[i][1] == 'c')
			code_flag = 1;
		if (argv[i][1] == 's')
			table_flag = 1;
		if (argv[i][1] == 'v')
			trace_flag = 1;
	}
	
	// lexical analysis
	list = lex_analyze(list_flag, input);
	// if there were any errors, stop
	if (list == NULL)
	{
		free(input);
		return 0;
	}
	
	// parsing
	code = parse(code_flag, table_flag, list);
	// if there were any errors, stop
	if (code == NULL)
	{
		free(input);
		free(list);
		return 0;
	}
	
	// execute code
	execute(trace_flag, code);
	
	free(input);
	free(list);
	free(code);
	return 0;
}