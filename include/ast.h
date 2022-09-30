#include "lexer.h"

typedef struct Eps_AstPrimaryNode Eps_AstPrimaryNode;
typedef struct Eps_AstUnaryNode Eps_AstUnaryNode;
typedef struct Eps_AstBinNode Eps_AstBinNode;
typedef struct Eps_AstTernaryNode Eps_AstTernaryNode;
typedef struct Eps_AstNode Eps_AstNode;

typedef enum {
    NODE_TERNARY = 0,
    NODE_BIN,
    NODE_UNARY,
    NODE_PRIMARY,
} Eps_AstNodeType;

struct Eps_AstNode {
    Eps_AstNodeType type;

    union {
        Eps_AstPrimaryNode *primary;
        Eps_AstUnaryNode   *unary;
        Eps_AstBinNode     *binary;
        Eps_AstTernaryNode *ternary;
    };

#ifdef EPS_DBG
    char *debug_string;
#endif
};

typedef struct Eps_Call {
    Eps_Token *identifier;
    EpsList   *args;
} Eps_Call;

struct Eps_AstPrimaryNode {
    enum {
        PRIMARY_LIT = 0,
        PRIMARY_PAREN,
        PRIMARY_CALL,
        PRIMARY_ID
    } type;

    union {
        Eps_Call    *func;
        Eps_Token   *identifier;
        Eps_Object  *literal;
        Eps_AstNode *expr; // for parenthesized expressions
    };
};

struct Eps_AstUnaryNode {
    Eps_AstNode *right;
    Eps_Token   *operator;
};

struct Eps_AstBinNode {
    Eps_AstNode *left;
    Eps_AstNode *right;
    Eps_Token   *operator;
};

struct Eps_AstTernaryNode {
    Eps_AstNode *cond;
    Eps_AstNode *left;
    Eps_AstNode *right;
};
