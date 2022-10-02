#include "interpreter/interpret.h"
#include "interpreter/enviroment.h"
#include "interpreter/statements.h"
#include "parser.h"
#include "core/memory.h"
#include "core/debug_macros.h"
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
        // Free statement result
        EpsMem_Free(
            Eps_RunStatement(env, (Eps_Statement *)stmt->data)
        );
        stmt = stmt->next;
    }
}
