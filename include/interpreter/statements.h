#ifndef _STATEMENTS_H
#   define _STATEMENTS_H

#include "interpreter/enviroment.h"
#include "parser.h"
#include "core/object.h"

typedef struct {
    enum {
        STMT_RES_RET = 0,
    } type;

    union {
        struct {
            Eps_StatementReturn *stmt;
            Eps_Object *val;
        } ret;
    };
} StmtResult;

StmtResult *
Eps_RunStatement(Eps_Env *env, Eps_Statement *stmt);

#endif
