#define ARRAY_SIZE 500

typedef enum token_type {
	identifier = 1, number, keyword_const, keyword_var, keyword_procedure,
	keyword_call, keyword_begin, keyword_end, keyword_if, keyword_then, 
	keyword_while, keyword_do, keyword_read, keyword_write, keyword_return,
	keyword_def, period, assignment_symbol, minus, semicolon,
	left_curly_brace, right_curly_brace, equal_to, not_equal_to, less_than,
	less_than_or_equal_to, greater_than, greater_than_or_equal_to, plus, times,
	division, left_parenthesis, right_parenthesis
} token_type;

typedef enum opcode_name {
	LIT = 1, OPR = 2, LOD = 3, STO = 4, CAL = 5, RTN = 6, INC = 7, JMP = 8, JPC = 9, 
	SYS = 10, WRT = 1, RED = 2, HLT = 3, 
	ADD = 1, SUB = 2, MUL = 3, DIV = 4, EQL = 5, NEQ = 6,
	LSS = 7, LEQ = 8, GTR = 9, GEQ = 10
} opcode_name;

typedef struct lexeme {
	token_type type;
	char identifier_name[12];
	int number_value;
	int error_type;
} lexeme;

typedef struct instruction {
	int op;
	int l;
	int m;
} instruction;

typedef struct symbol {
	int kind;
	char name[12];
	int value;
	int level;
	int address;
	int mark;
} symbol;

lexeme *lex_analyze(int list_flag, char *input);
instruction *parse(int code_flag, int table_flag, lexeme *list);
void execute(int trace_flag, instruction *code);