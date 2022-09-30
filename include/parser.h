#ifndef EPS_PARSER
#   define EPS_PARSER

#include "ast.h"
#include "core/object.h"

typedef Eps_AstNode Eps_Expression;

typedef struct Eps_Statement Eps_Statement;

typedef enum {
    S_EXPR = 0,
    S_GROUP,
    S_FUNC,
    S_RETURN,
    S_CONST,
    S_DEFINE,
    S_ASSIGN,
    S_IF,
    S_OUTPUT,
} Eps_StatementType;

#ifdef EPS_DBG
const char *
_EpsDbg_GetStmtTypeString(Eps_StatementType stmt_type);
#endif

typedef struct {
    Eps_Expression *expr;
} Eps_StatementExpr;

typedef EpsList Eps_StatementGroup;

typedef struct {
    Eps_Token      *identifier; // variable identifier
    Eps_Expression *expr;       // expression value to assign
    Eps_ObjectType  type;       // variable value type
    Eps_Token      *keyword;
} Eps_StatementVar;

typedef struct {
    Eps_Token      *identifier; // function identifier
    EpsList        *params;     // function parameters
    Eps_Statement  *body;
    Eps_ObjectType  type;       // return value type
    Eps_Token      *keyword;
} Eps_StatementFunc;

typedef struct {
    Eps_Expression *expr; // expression value to return
    Eps_Token      *keyword;
} Eps_StatementReturn;

typedef struct {
    Eps_Expression *expr; // expression value to output
    Eps_Token      *keyword;
} Eps_StatementOutput;

typedef struct {
    Eps_Expression *cond;  // statment condition
    Eps_Statement  *body;  // statement body
    Eps_Statement  *_else; // else statement
    Eps_Token      *keyword;
} Eps_StatementConditional;

struct Eps_Statement {
    Eps_StatementType type;

    union {
        Eps_StatementExpr           *expr;
        Eps_StatementOutput         *output;
        Eps_StatementConditional    *conditional;
        Eps_StatementGroup          *group;
        Eps_StatementVar            *define;
        Eps_StatementVar            *assign;
        Eps_StatementFunc           *func;
        Eps_StatementReturn         *ret;
    };
};

// Represents code as AST
EpsList *Eps_Parse(EpsList *tokens);

#endif
