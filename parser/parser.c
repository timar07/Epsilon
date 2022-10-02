#include "parser.h"
#include "core/object.h"
#include "core/memory.h"
#include "core/debug_macros.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

typedef struct {
    EpsList      *tokens;
    EpsList      *statements;

    Eps_Token    *current_tok;
    EpsList_Node *current_node;
} Parser;

// * - Core Debug Utils
#ifdef EPS_DBG
static const char * _EpsDbg_StmtTypeStrings[] = {
    "EXPR",
    "GROUP",
    "FUNC",
    "RETURN",
    "CONST",
    "DEFINE",
    "ASSIGN",
    "IF",
    "OUTPUT"
};

const char *
_EpsDbg_GetStmtTypeString(Eps_StatementType stmt_type)
{
    return _EpsDbg_StmtTypeStrings[stmt_type];
}
#endif

// * - Errors -

static void
syntax_error(Parser *self, const char format[], ...)
{
    ERR_INSTANCE_INIT_BUFFER();

    EpsErr_Raise(
        &self->current_tok->ls,
        "Syntax Error",
        buffer
    );
}

static void
syntax_error_expected(Parser *self, Eps_TokenType exp_tok)
{
    syntax_error(
        self,
        "expected %s instead of '%s'",
        _EpsDbg_GetTokenTypeString(exp_tok),
        self->current_tok->lexeme
    );
}

// * - Utils -

// Returns Current Token
static Eps_Token *
current(Parser *self)
{
    self->current_tok = (Eps_Token*)self->current_node->data;

    return self->current_tok;
}

// Returns Previous Token
static Eps_Token *
prev(Parser *self)
{
    return (Eps_Token *)self->current_node->prev->data;
}

// Returns Next Token
static Eps_Token *
peek_next(Parser *self)
{
    return (Eps_Token *)self->current_node->next->data;
}

// Returns current token and shifts the pointer
// to the next node.
static Eps_Token *
advance(Parser *self)
{
    Eps_Token *t = current(self);

    self->current_node = self->current_node->next;

    return t;
}

// Check if the current token matches 'token'
static bool
check(Parser *self, Eps_TokenType tok)
{
    return current(self)->toktype == tok;
}

// All the same as previous, but consumes the token
// if it matches
static bool
match(Parser *self, Eps_TokenType tok)
{
    if (check(self, tok)) {
        advance(self);
        return true;
    }

    return false;
}

// Synchronization routine
// FIXME: segfault?!
static void
synchronize(Parser *self)
{
    advance(self);

    while (current(self)->toktype != T_EOF) {
        if (prev(self)->toktype == SEMICOLON)
            return;

        switch (peek_next(self)->toktype) {
            case OUTPUT:
            case IF:
            case LET:
            case CONST:
            case FUNC:
                return;
            default:
                break;
        }

        advance(self);
    }
}

// Parse required token, otherwise raises a syntax error
static Eps_Token *
parse_required(Parser *self, Eps_TokenType tok)
{
    if (match(self, tok)) return prev(self);

    syntax_error_expected(self, tok);
    synchronize(self);

    return current(self);
}

// Look 'n' tokens ahead for 'tok'
static bool
lookahead(Parser *self, size_t n, Eps_TokenType tok)
{
    size_t i;
    EpsList_Node *current = self->current_node;

    for (i = 0; i < n; i++) {
        current = current->next;
    }

    return ((Eps_Token *)current->data)->toktype == tok;
}

// Check if token is valid type specifier
static bool
is_type_specifier(Eps_TokenType tok)
{
    switch (tok) {
        case REAL: case STRING:
        case BOOL: case VOID:
            return true;
        default: break;
    }

    return false;
}

// * - Expressions Constructors -

static Eps_Expression*
create_expression()
{
    return EpsMem_Alloc(sizeof(Eps_Expression));
}

static Eps_Expression *
create_ternary_node(Eps_Expression *cond, Eps_Expression *left,
                                          Eps_Expression *right)
{
    Eps_Expression* node = create_expression();

    node->type = NODE_TERNARY;
    node->ternary = EpsMem_Alloc(sizeof(Eps_AstTernaryNode));
    node->ternary->cond = cond;
    node->ternary->left = left;
    node->ternary->right = right;

    return node;
}

static Eps_Expression *
create_bin_node(Eps_Token *operator, Eps_Expression *left,
                                     Eps_Expression *right)
{
    Eps_Expression* node = create_expression();

    node->type = NODE_BIN;
    node->binary = EpsMem_Alloc(sizeof(Eps_AstBinNode));
    node->binary->operator = operator;
    node->binary->left = left;
    node->binary->right = right;

    return node;
}

static Eps_Expression *
create_unary_node(Eps_Token* operator, Eps_Expression *right)
{
    Eps_Expression *node = create_expression();

    node->type = NODE_UNARY;
    node->unary = EpsMem_Alloc(sizeof(Eps_AstUnaryNode));
    node->unary->operator = operator;
    node->unary->right = right;

    return node;
}

// Primary
static Eps_AstPrimaryNode *
create_literal_node(Eps_Object *literal)
{
    Eps_AstPrimaryNode *node = EpsMem_Alloc(sizeof(Eps_AstPrimaryNode));

    node->literal = literal;
    node->type = PRIMARY_LIT;

    return node;
}

static Eps_AstPrimaryNode *
create_parenthesized_node(Eps_Expression *expr)
{
    Eps_AstPrimaryNode *node = EpsMem_Alloc(sizeof(Eps_AstPrimaryNode));

    node->expr = expr;
    node->type = PRIMARY_PAREN;

    return node;
}

static Eps_AstPrimaryNode*
create_identifier_node(Eps_Token *identifier)
{
    Eps_AstPrimaryNode* node = EpsMem_Alloc(sizeof(Eps_AstPrimaryNode));

    node->identifier = identifier;
    node->type = PRIMARY_ID;

    return node;
}

static Eps_AstPrimaryNode*
create_call_node(Eps_Token *identifier, EpsList *args)
{
    Eps_AstPrimaryNode* node = EpsMem_Alloc(sizeof(Eps_AstPrimaryNode));

    node->type = PRIMARY_CALL;
    node->func = EpsMem_Alloc(sizeof(Eps_Call));
    node->func->identifier = identifier;
    node->func->args = args;

    return node;
}

#ifdef EPS_DBG
static char *
expr_to_string(Eps_Expression *expr)
{
    char *result = EpsMem_Alloc(sizeof(char)*80);

    switch (expr->type) {
        case NODE_PRIMARY:
        {
            switch (expr->primary->type) {
                case PRIMARY_LIT:
                {
                    switch (expr->primary->literal->type) {
                        case OBJ_BOOL:
                            sprintf(
                                result,
                                "%s",
                                *(bool *)expr->primary->literal->value ?
                                    "true": "false"
                            );
                        break;
                        case OBJ_REAL:
                            sprintf(
                                result,
                                "%f",
                                *(double *)expr->primary->literal->value
                            );
                        break;
                        case OBJ_STRING:
                            sprintf(
                                result,
                                "%s",
                                (char *)expr->primary->literal->value
                            );
                        break;
                    }
                    sprintf(
                        result,
                        "%f",
                        *(double *)expr->primary->literal->value
                    );
                } break;
                case PRIMARY_ID:
                {
                    sprintf(
                        result,
                        "%s",
                        expr->primary->identifier->lexeme
                    );
                } break;
                case PRIMARY_CALL:
                {
                    sprintf(
                        result,
                        "%s",
                        expr->primary->func->identifier->lexeme
                    );
                } break;
                default:
                    return expr_to_string(expr->primary->expr);
            }
        } break;
        case NODE_UNARY:
        {
            sprintf(
                result,
                "(%s %s)",
                _EpsDbg_GetTokenTypeString(expr->unary->operator->toktype),
                expr_to_string(expr->unary->right)
            );
        } break;
        case NODE_BIN:
        {
            sprintf(
                result,
                "(%s %s %s)",
                expr_to_string(expr->binary->left),
                _EpsDbg_GetTokenTypeString(expr->binary->operator->toktype),
                expr_to_string(expr->binary->right)
            );
        } break;
        case NODE_TERNARY:
        {
            sprintf(
                result,
                "(%s IF %s ELSE %s)",
                expr_to_string(expr->ternary->left),
                expr_to_string(expr->ternary->cond),
                expr_to_string(expr->ternary->right)
            );
        } break;
        default:
            return NULL;
    }

    return result;
}

static void
print_expression(Eps_Expression *expr)
{
    printf("%s\n", expr_to_string(expr));
}
#endif

// * - Parsing Utils -

// Parse string from token->lexeme
static Eps_Object *
parse_string(Eps_Token *token)
{
    size_t len = strlen(token->lexeme)-2;
    char *literal = EpsMem_Alloc(sizeof(char)*(len+1));
    memcpy(literal, &token->lexeme[1], len);
    token->lexeme[len+1] = '\0';

    return EpsObject_Create(OBJ_STRING, literal, false);
}

// Parse number from token->lexeme
static Eps_Object *
parse_number(Eps_Token *token)
{
    double *val = EpsMem_Alloc(sizeof(double));

    *val = strtod(token->lexeme, NULL);

    return EpsObject_Create(OBJ_REAL, val, false);
}

// Parse type specifier
static Eps_ObjectType
parse_type_spec(Eps_Token *token)
{
    switch (token->toktype) {
        case REAL:
            return OBJ_REAL;
        default:
            return OBJ_VOID;
    }
}

// * - Parsing Expressions -

static Eps_Expression *
expression(Parser *self);

static Eps_AstPrimaryNode *
parse_call(Parser *self)
{
    // Function call matches following grammary:
    // call = identifier '(' args ')';
    // args = arg | (arg ',' args);

    Eps_Token *identifier = advance(self);
    EpsList *args = EpsList_Create();

    parse_required(self, L_PAREN);
    while (!match(self, R_PAREN)) {
        EpsList_Append(args, expression(self));

        if (!check(self, R_PAREN)) {
            parse_required(self, COMMA);
        }
    }

    return create_call_node(identifier, args);
}

static Eps_Expression *
primary(Parser *self)
{
    if (match(self, T_EOF)) {
        EpsErr_Fatal("unexpected end of file");
    }

    Eps_Expression* expr = create_expression();
    expr->type = NODE_PRIMARY;

    if (check(self, NUMBER)) {
        expr->primary = create_literal_node(
            parse_number(advance(self))
        );
    }
    else if(check(self, STRING)) {
        expr->primary = create_literal_node(
            parse_string(advance(self))
        );
    }
    else if(check(self, IDENTIFIER)) {
        if(lookahead(self, 1, L_PAREN))
            expr->primary = parse_call(self);
        else
            expr->primary = create_identifier_node(advance(self));
    }
    else if (match(self, VOID)) {
        expr->primary = create_literal_node(NULL);
    }
    else if (check(self, TRUE) || check(self, FALSE)) {
        Eps_Token* t = advance(self);
        bool* val = EpsMem_Alloc(sizeof(bool));

        if (t->toktype == TRUE) {
            *val = true;
        } else {
            *val = false;
        }

        expr->primary = create_literal_node(
            EpsObject_Create(OBJ_BOOL, val, false)
        );
    }
    else if (match(self, L_PAREN)) {
        expr->primary = create_parenthesized_node(
            expression(self)
        );
        parse_required(self, R_PAREN);
    }
    else {
        syntax_error(self, "expected expression");
    }

    return expr;
}

static Eps_Expression*
unary(Parser *self)
{
    // unary = '-' primary;
    if (check(self, MINUS)) {
        Eps_Expression* expr = create_expression();

        Eps_Token* operator = advance(self);
        Eps_Expression* right = primary(self);

        expr = create_unary_node(operator, right);

        return expr;
    }

    return primary(self);
}

static Eps_Expression *
factor(Parser *self)
{
    // factor = unary (('*' | '/') unary)*;
    Eps_Expression *expr = unary(self);

    while (check(self, STAR) || check(self, SLASH)) {
        Eps_Token *operator = advance(self);
        Eps_Expression *right = unary(self);

        expr = create_bin_node(operator, expr, right);
    }

    return expr;
}

static Eps_Expression*
term(Parser *self)
{
    // term = factor (('+' | '-') factor)*;
    Eps_Expression *expr = factor(self);

    while (check(self, PLUS) || check(self, MINUS)) {
        Eps_Token *operator = advance(self);
        Eps_Expression *right = factor(self);

        expr = create_bin_node(operator, expr, right);
    }

    return expr;
}

static Eps_Expression *
comparison(Parser *self)
{
    // comparison = term (('=' | '<' | '>' | '<=' | '>=') term)*;
    Eps_Expression *expr = term(self);

    while (check(self, EQUAL)      ||
           check(self, LESS)       ||
           check(self, GREATER)    ||
           check(self, LESS_EQUAL) ||
           check(self, GREATER_EQUAL)) {

        Eps_Token *operator = advance(self);
        Eps_Expression *right = term(self);

        expr = create_bin_node(operator, expr, right);
    }

    return expr;
}

static Eps_Expression *
equality(Parser *self)
{
    // equality = comparison (('!=' | '=') comparison)*;

    Eps_Expression *expr = comparison(self);

    while (check(self, BANG_EQUAL) ||
           check(self, EQUAL)) {
        Eps_Token *operator = advance(self);
        Eps_Expression *right = comparison(self);

        expr = create_bin_node(operator, expr, right);
    }

    return expr;
}

static Eps_Expression *
ternary(Parser *self)
{
    // ternary = equality 'if' equality 'else' ternary | equality;
    Eps_Expression *left = equality(self);

    if (match(self, IF)) {
    _DEBUG("ternary\n");
        Eps_Expression *condition = equality(self);

        parse_required(self, ELSE);
        return create_ternary_node(condition, left, ternary(self));
    }

    return left;
}

static Eps_Expression *
expression(Parser *self)
{
    Eps_Expression *expr;
    Eps_LexState ls;

    ls = current(self)->ls; // saving current token lexstate
    expr = ternary(self);   // parsing

    // include only one line of expression
    if (prev(self)->ls.line == ls.line) {
        // expression end is an enclosing token end
        ls.end = prev(self)->ls.end;
    }
    expr->ls = ls; // attaching lexstate to the expression

#ifdef EPS_DBG
    expr->debug_string = expr_to_string(expr);
#endif

    _DEBUG("DEBUG STRING: %s\n", expr->debug_string);

    return expr;
}


// * - Parsing Statements -

static Eps_Statement *
create_statement(void)
{
    return EpsMem_Alloc(sizeof(Eps_Statement));
}

static Eps_Statement *
statement(Parser *self);

static Eps_Statement *
stmt_group(Parser *self)
{
    // Statement group matches following grammary:
    // group = '{' statement* '}';

    Eps_Statement *stmt = create_statement();

    stmt->type = S_GROUP;
    stmt->group = EpsMem_Alloc(sizeof(Eps_StatementGroup));
    stmt->group = EpsList_Create();

    parse_required(self, L_BRACE);
    while (!match(self, R_BRACE)) {
        EpsList_Append(stmt->group, statement(self));
    }
    return stmt;
}

static Eps_Statement *
stmt_expr(Parser *self)
{
    // Expression statement matches following grammary:
    // stmt_expr = expression ';';

    Eps_Statement *stmt = create_statement();

    stmt->type = S_EXPR;
    stmt->expr = EpsMem_Alloc(sizeof(Eps_StatementExpr));
    stmt->expr->expr = expression(self);

    parse_required(self, SEMICOLON);

    return stmt;
}

static Eps_Statement *
stmt_func(Parser *self)
{
    // Function definition statement matches following grammary:
    // func = 'func' identifier '(' params ')' '->' type statement;
    // params = param | (param | "," params);
    // param  = identifier ':' type;

    Eps_Statement *stmt = create_statement();

    stmt->type = S_FUNC;
    stmt->func = EpsMem_Alloc(sizeof(Eps_StatementFunc));
    stmt->func->keyword = parse_required(self, FUNC);
    stmt->func->identifier = advance(self);
    stmt->func->params = EpsList_Create();

    parse_required(self, L_PAREN);

    while (!match(self, R_PAREN)) {
        EpsList_Append(stmt->func->params, advance(self));

        parse_required(self, COLON);
        advance(self);

        if (!check(self, R_PAREN)) {
            parse_required(self, COMMA);
        }
    }

    parse_required(self, ARROW_RIGHT);

    if(is_type_specifier(current(self)->toktype)) {
        stmt->func->type = parse_type_spec(advance(self));
        stmt->func->body = statement(self);
    } else {
        syntax_error(self, "expected function type specifier");
    }

    return stmt;
}

static Eps_Statement *
stmt_return(Parser *self)
{
    // Return statement matches following grammary:
    // return = 'return' expression ';';

    Eps_Statement *stmt = create_statement();

    stmt->type = S_RETURN;
    stmt->ret = EpsMem_Alloc(sizeof(Eps_StatementReturn));
    stmt->ret->keyword = parse_required(self, RETURN);

    if(!match(self, SEMICOLON)) {
        stmt->ret->expr = expression(self);
        parse_required(self, SEMICOLON);
    } else {
        stmt->ret->expr = NULL;
    }

    return stmt;
}

static Eps_Statement *
stmt_const(Parser *self)
{
    // Constant definition matches following grammary:
    // const = 'const' identifier ':' type_specifier '<-' expression ';';

    Eps_Statement *stmt = create_statement();

    stmt->type = S_CONST;
    stmt->define = EpsMem_Alloc(sizeof(Eps_StatementVar));
    stmt->define->keyword = parse_required(self, CONST);
    stmt->define->identifier = advance(self);
    parse_required(self, COLON);
    stmt->define->type = parse_type_spec(advance(self));
    parse_required(self, ARROW_LEFT);
    stmt->define->expr = expression(self);
    parse_required(self, SEMICOLON);

    return stmt;
}

static Eps_Statement *
stmt_define(Parser *self)
{
    // Variable definition matches following grammary:
    // define = 'let' identifier ':' type_specifier '<-' expression ';';

    Eps_Statement *stmt = create_statement();

    stmt->type = S_DEFINE;
    stmt->define = EpsMem_Alloc(sizeof(Eps_StatementVar));
    stmt->define->keyword = parse_required(self, LET);
    stmt->define->identifier = advance(self);
    parse_required(self, COLON);
    stmt->define->type = parse_type_spec(advance(self));
    parse_required(self, ARROW_LEFT);
    stmt->define->expr = expression(self);
    parse_required(self, SEMICOLON);

    return stmt;
}

static Eps_Statement *
stmt_assign(Parser *self)
{
    // Variable assignment matches following grammary:
    // assign = identifier '<-' expression ';';

    if(!lookahead(self, 1, ARROW_LEFT))
        return stmt_expr(self);


    Eps_Statement *stmt = create_statement();

    stmt->type = S_ASSIGN;
    stmt->assign = EpsMem_Alloc(sizeof(Eps_StatementVar));
    stmt->assign->identifier = advance(self);
    parse_required(self, ARROW_LEFT);
    stmt->assign->expr = expression(self);
    parse_required(self, SEMICOLON);

    return stmt;
}

static Eps_Statement *
stmt_output(Parser *self)
{
    // Output statement matches following grammary:
    // output = 'output' expression ';';

    Eps_Statement *stmt = create_statement();

    stmt->type = S_OUTPUT;
    stmt->output = EpsMem_Alloc(sizeof(Eps_StatementOutput));
    stmt->output->keyword = parse_required(self, OUTPUT);
    stmt->output->expr = expression(self);

    parse_required(self, SEMICOLON);

    return stmt;
}

static Eps_Statement *
stmt_if(Parser *self)
{
    // If statement matches following grammary:
    // if = 'if' expression statement;

    Eps_Statement *stmt = create_statement();

    stmt->type = S_IF;
    stmt->conditional = EpsMem_Alloc(sizeof(Eps_StatementConditional));
    stmt->conditional->keyword = parse_required(self, IF);
    stmt->conditional->cond = expression(self);
    stmt->conditional->body = statement(self);

    if (match(self, ELSE)) {
        stmt->conditional->_else = statement(self);
    }

    return stmt;
}

static Eps_Statement *
statement(Parser *self)
{
    Eps_Statement *stmt;

    switch (current(self)->toktype) {
        case L_BRACE:
        {
            stmt = stmt_group(self);
        } break;
        case OUTPUT:
        {
            stmt = stmt_output(self);
        } break;
        case IF:
        {
            stmt = stmt_if(self);
        } break;
        case FUNC:
        {
            stmt = stmt_func(self);
        } break;
        case RETURN:
        {
            stmt = stmt_return(self);
        } break;
        case CONST:
        {
            stmt = stmt_const(self);
        } break;
        case LET:
        {
            stmt = stmt_define(self);
        } break;
        case IDENTIFIER:
        {
            stmt = stmt_assign(self);
        } break;
        default:
        {
            stmt = stmt_expr(self);
        }
    }

    _DEBUG("<STMT TYPE=%s>\n", _EpsDbg_GetStmtTypeString(stmt->type));

    return stmt;
}

EpsList *
Eps_Parse(EpsList *tokens)
{

    _DEBUG("----------------- PARSER: -----------------\n");

    Parser self;
    self.tokens = tokens;
    self.current_node = tokens->head;
    self.statements = EpsList_Create();
    self.current_tok = NULL;

    while (current(&self)->toktype != T_EOF) {
        EpsList_Append(self.statements, statement(&self));
    }

    return self.statements;
}
