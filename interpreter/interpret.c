#include "interpreter/interpret.h"
#include "interpreter/enviroment.h"
#include "interpreter/statements.h"
#include "interpreter/runtime_errors.h"
#include "core/memory.h"
#include "core/debug_macros.h"
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

void
Eps_Interpret(EpsList *stmts)
{
    _DEBUG("--------------- INTERPRETER ---------------\n");

    EpsList_Node *stmt = stmts->head;
    Eps_Env *env = Eps_EnvCreate();

    while (!EpsErr_WasError() && stmt != NULL) {
        StmtResult *res = Eps_RunStatement(env, (Eps_Statement *)stmt->data);

        if (res != NULL && res->type == STMT_RES_RET) {
            EpsErr_RuntimeError(
                &res->ret.stmt->keyword->ls,
                "cannot return outside of the function"
            );
        }

        EpsMem_Free(res);
        stmt = stmt->next;
    }
}
