#ifndef EPS_LEXER
#	define EPS_LEXER

#include "core/object.h"
#include "core/ds/list.h"
#include "core/input.h"
#include <stddef.h> /* size_t */

typedef enum {
	L_PAREN = 0,
	R_PAREN,
	L_BRACE,
	R_BRACE,
	COMMA,
	DOT,
	MINUS,
	PLUS,
	OUTPUT,
	COLON,
	SEMICOLON,
	SLASH,
	STAR,
	BANG,
	BANG_EQUAL,
	EQUAL,
	GREATER,
	GREATER_EQUAL,
	LESS,
	LESS_EQUAL,
	IDENTIFIER,
	STRING,
	NUMBER,
	AND,
	CONST,
	FUNC,
	ELSE,
	FALSE,
	IF,
	LET,
	VOID,
	OR,
	RETURN,
	REAL,
	TRUE,
	ARROW_RIGHT,
	ARROW_LEFT,
	T_EOF,
	COMMENT,
	ERRORTOKEN,
} Eps_TokenType;

typedef struct {
    size_t line;
    size_t col;
	size_t start;
	size_t end;
	size_t current;
	char *fname;
	Eps_Input *input;
} Eps_LexState;

typedef struct {
    Eps_TokenType toktype;
    Eps_LexState ls;
    char *lexeme;
} Eps_Token;

// Warning: do not change the order
static char  *_EpsDbg_TokenTypeStrings[] = {
	"L_PAREN",
	"R_PAREN",
	"L_BRACE",
	"R_BRACE",
	"COMMA",
	"DOT",
	"MINUS",
	"PLUS",
	"OUTPUT",
	"COLON",
	"SEMICOLON",
	"SLASH",
	"STAR",
	"BANG",
	"BANG_EQUAL",
	"EQUAL",
	"GREATER",
	"GREATER_EQUAL",
	"LESS",
	"LESS_EQUAL",
	"IDENTIFIER",
	"STRING",
	"NUMBER",
	"AND",
	"CONST",
	"FUNC",
	"ELSE",
	"FALSE",
	"IF",
	"LET",
	"VOID",
	"OR",
	"RETURN",
	"REAL",
	"TRUE",
	"ARROW_RIGHT",
	"ARROW_LEFT",
	"<EOF>",
	"<COMMENT>",
	"<ERRORTOKEN>",
};

char *
_EpsDbg_GetTokenTypeString(Eps_TokenType toktype);

void
_EpsDbg_TokenDump(Eps_Token *tok);

void
Eps_DestroyToken(Eps_Token *tok);

EpsList *
Eps_Lex(Eps_Input* input);

#endif
