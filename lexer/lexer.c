#include "lexer/lexer.h"
#include "lexer/token.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/ds/list.h"
#include "core/errors.h"
#include "core/debug_macros.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>


#define lexeme_cmp(a, b) (strcmp((a), (b)) == 0)

// * - Utils -

// Return and consume raw character
static int
get_char(Eps_LexState *ls)
{
    if (ls->current >= ls->input->len)
        return EOF;

    int c = ls->input->raw[ls->current++];

    if (c == '\n') {
        ls->line++;
        ls->col = 0;
    }

    ls->col++;

    return c;
}

// Return char at position 'pos'
static int
char_at(Eps_LexState *ls, size_t pos)
{
    if (pos >= ls->input->len)
        return EOF;

    return ls->input->raw[pos];
}

// Return and consume the character
static int
advance(Eps_LexState *ls)
{
    int c;

    while (isspace(c = get_char(ls)) && c != EOF)
        ;

    ls->end = ls->current;

    return c;
}

// Return current char
static int
current(Eps_LexState *ls)
{
    return char_at(ls, ls->current);
}

// Check if current char matches expected 'c',
// if so, consume the character
static bool
match(Eps_LexState *ls, char c)
{
    if (current(ls) == c) {
        advance(ls);
        return true;
    }

    return false;
}

// Return substring from a current file
static char *
get_substr(Eps_LexState *ls, size_t start, size_t end)
{
    size_t strsz = sizeof(char)*end-start+1;
    char *substr = EpsMem_Alloc(strsz);

    memcpy(substr, &ls->input->raw[start], strsz-1);
    substr[strsz] = '\0';

    return substr;
}

// Create token from LexState and TokenType
static Eps_Token *
create_token(Eps_LexState *ls, Eps_TokenType toktype)
{
    return Eps_CreateToken(
        ls,
        toktype,
        get_substr(ls, ls->start, ls->end)
    );
}

// * - Lexical Errors -

static void
lexerror(Eps_LexState *ls, char format[], ...)
{
    ERR_INSTANCE_INIT_BUFFER();
    EpsErr_Raise(ls, "Lexical error", buffer);
}

// * - Lexer Utils -

static void
line_comment(Eps_LexState *ls)
{
    int c = get_char(ls);

    while (c != '\n' && c != EOF)
        c = get_char(ls);
}

// Parse number
static Eps_Token *
number(Eps_LexState *ls)
{
    int c = current(ls);

    while (isdigit(c) || c == '.') {
        if (c == '.' && !isdigit(char_at(ls, ls->end+1))) {
            lexerror(ls, "expected digit after decimal point");
        }

        advance(ls);
        c = current(ls);
    }

    return create_token(ls, NUMBER);
}

// Parse string token
static Eps_Token *
string(Eps_LexState *ls)
{
    int c = advance(ls);

    while (c != '"' && c != '\n') {
        if (c == EOF) {
            lexerror(ls, "unterminated string");
            break;
        }

        c = advance(ls);
    }

    return create_token(ls, STRING);
}

// TODO: Optimize
static Eps_TokenType
identifier(Eps_LexState *ls)
{
    size_t start = ls->end-1;
    char* lexeme;

    while (isalnum(current(ls)))
        advance(ls);

    lexeme = get_substr(ls, start, ls->end);

    /* Checking if that's an keyword */
    if (lexeme_cmp(lexeme, "and")) {
        return AND;
    }
    if (lexeme_cmp(lexeme, "const")) {
        return CONST;
    }
    else if (lexeme_cmp(lexeme, "func")) {
        return FUNC;
    }
    else if (lexeme_cmp(lexeme, "else")) {
        return ELSE;
    }
    else if (lexeme_cmp(lexeme, "false")) {
        return FALSE;
    }
    else if (lexeme_cmp(lexeme, "if")) {
        return IF;
    }
    else if (lexeme_cmp(lexeme, "let")) {
        return LET;
    }
    else if (lexeme_cmp(lexeme, "void")) {
        return VOID;
    }
    else if (lexeme_cmp(lexeme, "or")) {
        return OR;
    }
    else if (lexeme_cmp(lexeme, "not")) {
        return NOT;
    }
    else if (lexeme_cmp(lexeme, "output")) {
        return OUTPUT;
    }
    else if (lexeme_cmp(lexeme, "return")) {
        return RETURN;
    }
    else if (lexeme_cmp(lexeme, "real")) {
        return REAL;
    }
    else if (lexeme_cmp(lexeme, "bool")) {
        return BOOL;
    }
    else if (lexeme_cmp(lexeme, "true")) {
        return TRUE;
    }
    else { /* otherwise, that's an keyword */
        return IDENTIFIER;
    }
}

static Eps_Token *
get_token(Eps_LexState *ls)
{
    Eps_Token *t;
    int c;

    c = advance(ls);
    ls->start = ls->end-1;

    switch (c) {
        case '(':
			t = create_token(ls, L_PAREN);
		break;
		case ')':
			t = create_token(ls, R_PAREN);
		break;
		case '{':
			t = create_token(ls, L_BRACE);
		break;
		case '}':
			t = create_token(ls, R_BRACE);
		break;
		case ',':
			t = create_token(ls, COMMA);
		break;
		case '.':
			t = create_token(ls, DOT);
		break;
        case '-':
        {
            if (match(ls, '>')) { // ->
                t = create_token(ls, ARROW_RIGHT);
            } else if (match(ls, '-')) { // --
                line_comment(ls);
                t = get_token(ls);
            } else {
			    t = create_token(ls, MINUS);
            }
        } break;
        case '+':
			t = create_token(ls, PLUS);
		break;
        case ':':
			t = create_token(ls, COLON);
		break;
		case ';':
			t = create_token(ls, SEMICOLON);
		break;
		case '*':
			t = create_token(ls, STAR);
		break;
        case '=':
			t = create_token(ls, EQUAL);
        break;

		case '!':
		{
			if (match(ls, '=')) {
                // !=
				t = create_token(ls, BANG_EQUAL);
			} else {
				t = create_token(ls, BANG);
			}
        } break;
		case '<':
		{
			if (match(ls, '=')) {
                // <=
				t = create_token(ls, LESS_EQUAL);
			} else if (match(ls, '-')) {
                t = create_token(ls, ARROW_LEFT);
            } else {
				t = create_token(ls, LESS);
			}
        } break;
		case '>':
		{
			if (match(ls, '=')) {
                // >=
				t = create_token(ls, GREATER_EQUAL);
			} else {
				t = create_token(ls, GREATER);
			}
        } break;
		case '/':
		{
            t = create_token(ls, SLASH);
        } break;
        case '"':
        {
            t = string(ls);
        } break;
        case EOF:
        {
            t = create_token(ls, T_EOF);
        } break;
        default:
        {
            if (isdigit(c)) { // is it a number literal?
                t = number(ls);
            } else if (isalpha(c)) { // is it a keyword/identifier?
                t = create_token(ls, identifier(ls));
            } else { // otherwise, we don't know what it is
                t = create_token(ls, ERRORTOKEN);
                lexerror(&t->ls, "illegal token '%s'", t->lexeme);
            }
        } break;
    }

    return t;
}

EpsList *
Eps_Lex(Eps_Input *input)
{
    Eps_LexState ls;
    Eps_Token *t;
    EpsList *tokens;

    ls.fname = input->name;
    ls.input = input;
    ls.line = 1;
    ls.col = 0;
    ls.start = 0;
    ls.end = 0;
    ls.current = 0;

    tokens = EpsList_Create();

    _DEBUG("----------------- LEXER -----------------\n");

    do {
        t = get_token(&ls);
        EpsList_Append(tokens, t);
#ifdef EPS_DBG
        _EpsDbg_TokenDump(t);
#endif
    } while (t->toktype != T_EOF);

    return tokens;
}
