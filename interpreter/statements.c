#include "interpreter/statements.h"
#include "interpreter/expressions.h"
#include "interpreter/enviroment.h"
#include "interpreter/runtime_errors.h"
#include "core/debug_macros.h"
#include "core/errors.h"
#include "core/memory.h"
#include <stdarg.h>

// * - Utils -
static StmtResult *
stmt_res_create(void)
{
    return EpsMem_Alloc(sizeof(StmtResult));
}

static StmtResult *
stmt_res_return(Eps_Object *val, Eps_StatementReturn *stmt)
{
    StmtResult *stmt_res = stmt_res_create();

    stmt_res->type = STMT_RES_RET;
    stmt_res->ret.val = val;
    stmt_res->ret.stmt = stmt;

    return stmt_res;
}

// * - Running Statements -
static StmtResult *
visit_expr_stmt(Eps_Env *env, Eps_StatementExpr *stmt)
{
    Eps_EvalExpr(env, stmt->expr);

    return NULL;
}

static StmtResult *
visit_group(Eps_Env *env, Eps_StatementGroup *stmt)
{
    EpsList_Node *node = stmt->head;
    Eps_Env *block_env = Eps_EnvCreate();
    StmtResult *res;

    block_env->scope = SCOPE_BLOCK;
    block_env->enclosing = env;

    // while we didn't found return statement
    while (node != NULL) {
        res = Eps_RunStatement(block_env, node->data);
        node = node->next; // keep moving on

        if (res != NULL) return res;
    }

    // Eps_EnvDestroy(block_env);

    return NULL;
}

static StmtResult *
visit_if(Eps_Env *env, Eps_StatementConditional *stmt)
{
    Eps_Object *cond = Eps_EvalExpr(env, stmt->cond);

    if (cond->type != OBJ_BOOL) {
        EpsErr_RuntimeError(
            &stmt->cond->ls,
            "invalid condition type '%s'",
            EpsDbg_GetObjectTypeString(cond->type)
        );

        EpsObject_Destroy(cond);
        return NULL;
    }


    if (*(double *)cond->value) {
        return Eps_RunStatement(env, stmt->body);
    } else if (stmt->_else != NULL) {
        return Eps_RunStatement(env, stmt->_else);
    }

    EpsObject_Destroy(cond);
    return NULL;
}

static StmtResult *
visit_output(Eps_Env *env, Eps_StatementOutput *stmt)
{
    Eps_Object *val = Eps_EvalExpr(env, stmt->expr);


    switch (val->type) {
        case OBJ_STRING:
            printf("%s\n", (char *)val->value);
        break;
        case OBJ_REAL:
            printf("%f\n", *(double *)val->value);
        break;
        case OBJ_BOOL:
            printf(
                "%s\n",
                *(bool *)val->value ? "true": "false"
            );
        break;
        case OBJ_VOID:
            EpsErr_RuntimeError(
                &stmt->expr->ls,
                "cannot output value type of 'void'"
            );
    }

    EpsObject_Destroy(val);

    return NULL;
}

static StmtResult *
visit_return(Eps_Env *env, Eps_StatementReturn *stmt)
{
    // Check if there is an expression in return statement
    if (stmt->expr != NULL)
        return stmt_res_return(Eps_EvalExpr(env, stmt->expr), stmt);

    return NULL;
}

static StmtResult *
visit_func(Eps_Env *env, Eps_StatementFunc *stmt)
{
    bool already_defined = Eps_EnvGet(env, stmt->identifier->lexeme) != NULL;

    if (!already_defined) {
        Eps_EnvDefine(
            env,
            stmt->identifier->lexeme,
            stmt
        );
    } else {
        EpsErr_RuntimeError(
            &stmt->identifier->ls,
            "function '%s' is already defined",
            stmt->identifier->lexeme
        );
    }

    return NULL;
}

static StmtResult *
visit_const(Eps_Env *env, Eps_StatementVar *stmt)
{
    if (!Eps_EnvGetLocal(env, stmt->identifier->lexeme)) {
        Eps_Object *val = Eps_EvalExpr(env, stmt->expr);

        // if const type matches value type
        if(val->type == stmt->type) {
            Eps_EnvDefine(
                env,
                stmt->identifier->lexeme,
                val
            );
        } else {
            EpsErr_RuntimeError(
                &stmt->identifier->ls,
                "cannot assign value type '%s' to const type '%s'",
                EpsDbg_GetObjectTypeString(val->type),
                EpsDbg_GetObjectTypeString(stmt->type)
            );
        }
    } else {
        EpsErr_RuntimeError(
            &stmt->identifier->ls,
            "constant '%s' is already defined",
            stmt->identifier->lexeme
        );
    }

    return NULL;
}

static StmtResult *
visit_define(Eps_Env *env, Eps_StatementVar *stmt)
{
    Eps_Object *temp = Eps_EnvGetLocal(env, stmt->identifier->lexeme);

    if (temp == NULL) {
        Eps_Object *val = Eps_EvalExpr(env, stmt->expr);

        // if variable type matches value type
        if(val->type == stmt->type) {
            Eps_EnvDefine(
                env,
                stmt->identifier->lexeme,
                val
            );
        } else {
            EpsErr_RuntimeError(
                &stmt->identifier->ls,
                "cannot assign value type '%s' to variable type '%s'",
                EpsDbg_GetObjectTypeString(val->type),
                EpsDbg_GetObjectTypeString(stmt->type)
            );
        }
    } else {
        EpsErr_RuntimeError(
            &stmt->identifier->ls,
            "varable '%s' is already defined",
            stmt->identifier->lexeme
        );
    }

    return NULL;
}

static StmtResult *
visit_assign(Eps_Env *env, Eps_StatementVar *stmt)
{
    Eps_Object *ref_val = Eps_EnvGet(env, stmt->identifier->lexeme);
    Eps_Object *new_val = Eps_EvalExpr(env, stmt->expr);
    bool is_impicit = ref_val == NULL;

    // check if implicit declaration
    if (is_impicit) {
        EpsErr_RuntimeError(
            &stmt->identifier->ls,
            "varable '%s' is not defined",
            stmt->identifier->lexeme
        );

        return NULL;
    }

    // check if types matches
    if (ref_val->type != new_val->type) {
        EpsErr_RuntimeError(
            &stmt->identifier->ls,
            "cannot assign '%s' to variable type '%s'",
            EpsDbg_GetObjectTypeString(new_val->type),
            EpsDbg_GetObjectTypeString(ref_val->type)
        );

        return NULL;
    }

    // if reference value is not mutable
    if(ref_val->mut == false) {
        EpsErr_RuntimeError(
            &stmt->identifier->ls,
            "cannot assign value to const '%s'",
            stmt->identifier->lexeme
        );

        return NULL;
    }

    EpsObject_Destroy(ref_val);
    *ref_val = *new_val;

    return NULL;
}

StmtResult *
Eps_RunStatement(Eps_Env *env, Eps_Statement *stmt)
{
    _DEBUG("STATEMENT: %s\n", _EpsDbg_GetStmtTypeString(stmt->type));

    switch (stmt->type) {
        case S_EXPR:
            return visit_expr_stmt(env, stmt->expr);
        case S_GROUP:
            return visit_group(env, stmt->group);
        case S_OUTPUT:
            return visit_output(env, stmt->output);
        case S_IF:
            return visit_if(env, stmt->conditional);
        case S_FUNC:
            return visit_func(env, stmt->func);
        case S_RETURN:
            return visit_return(env, stmt->ret);
        case S_CONST:
            return visit_const(env, stmt->define);
        case S_DEFINE:
            return visit_define(env, stmt->define);
        case S_ASSIGN:
            return visit_assign(env, stmt->assign);
        default: break;
    }

    return NULL;
}
