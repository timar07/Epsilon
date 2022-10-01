#include "interpret.h"
#include "enviroment.h"
#include "parser.h"
#include "lexer.h"
#include "core/memory.h"
#include "core/debug_macros.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// * - Private Types -
typedef struct {
    enum {
        STMT_RES_NOOP = 0,
        STMT_RES_RET,
    } type;

    union {
        struct {
            Eps_StatementReturn *stmt;
            Eps_Object *val;
        } ret;

        void *noop;
    };
} StmtResult;

static StmtResult *
stmt_res_create(void)
{
    return Eps_AllocMem(sizeof(StmtResult));
}

static StmtResult *
stmt_res_noop(void)
{
    StmtResult *stmt_res = stmt_res_create();

    stmt_res->type = STMT_RES_NOOP;
    stmt_res->noop = NULL;

    return stmt_res;
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

// * - Forwards -
static StmtResult *
statement(Eps_Env *env, Eps_Statement *stmt);

// * - Errors -
static void
runtime_error(Eps_LexState *ls, char *format, ...)
{
    ERR_INSTANCE_INIT_BUFFER();

    EpsErr_Raise(ls, "Runtime Error", buffer);

}

// * - Utils -
static Eps_Object *
create_number(double val)
{
    double *obj_val = Eps_AllocMem(sizeof(double));
    *obj_val = val;

    return Eps_ObjectCreate(OBJ_REAL, obj_val, true);
}

static Eps_Object *
create_boolean(bool val)
{
    double *obj_val = Eps_AllocMem(sizeof(bool));
    *obj_val = val;

    return Eps_ObjectCreate(OBJ_BOOL, obj_val, true);
}

static Eps_Object *
create_string(char *str)
{
    return Eps_ObjectCreate(OBJ_STRING, str, true);
}

static Eps_Object *
create_void()
{
    return Eps_ObjectCreate(OBJ_VOID, NULL, true);
}

// * - Evaluating Expressions -

static Eps_Object *
expression(Eps_Env *env, Eps_Expression* expr);

static Eps_Object *
visit_ternary(Eps_Env *env, Eps_AstTernaryNode* node)
{
    _DEBUG("%*sTERNARY\n", 8, "");
    Eps_Object *cond = expression(env, node->cond);

    if (cond->type != OBJ_BOOL) {
        // TODO: Throw runtime error
        return create_void();
    }

    if (*(double *)cond->value) {
        return expression(env, node->left);
    } else {
        return expression(env, node->right);
    }
}

static Eps_Object *
visit_binary(Eps_Env *env, Eps_AstBinNode* node)
{
    _DEBUG("%*sBINARY\n", 8, "");
    Eps_Object *left = expression(env, node->left);
    Eps_Object *right = expression(env, node->right);

    if (left->type == OBJ_VOID || right->type == OBJ_VOID) {
        return create_void();
    }

    if (left->type == OBJ_REAL && right->type == OBJ_REAL) {
        switch (node->operator->toktype) {
            case PLUS:
            {
                return create_number(
                    *(double *)left->value + *(double *)right->value
                );
            }

            case MINUS:
            {
                return create_number(
                    *(double *)left->value - *(double *)right->value
                );
            }

            case STAR:
            {
                return create_number(
                    *(double *)left->value * *(double *)right->value
                );
            }

            case SLASH:
            {
                return create_number(
                    *(double *)left->value / *(double *)right->value
                );
            }

            case EQUAL:
            {
                return create_boolean(
                    *(double *)left->value == *(double *)right->value
                );
            }

            case BANG_EQUAL:
            {
                return create_boolean(
                    *(double *)left->value != *(double *)right->value
                );
            }

            case LESS_EQUAL:
            {
                return create_boolean(
                    *(double *)left->value <= *(double *)right->value
                );
            }

            case GREATER_EQUAL:
            {
                return create_boolean(
                    *(double *)left->value >= *(double *)right->value
                );
            }

            case LESS:
            {
                return create_boolean(
                    *(double *)left->value < *(double *)right->value
                );
            }

            case GREATER:
            {
                return create_boolean(
                    *(double *)left->value > *(double *)right->value
                );
            }

            default: break;
        }
    }
    else if (left->type == OBJ_STRING && right->type == OBJ_STRING) {
        switch (node->operator->toktype) {
            case PLUS:
            {
                // concatenating strings
                char *buff = Eps_AllocMem(sizeof(char)*(strlen(left->value)+strlen(right->value)));
                sprintf(
                    buff,
                    "%s%s",
                    (char *)left->value,
                    (char *)right->value
                );

                return create_string(buff);
            }
            case MINUS: case STAR:
            case SLASH:
            {
                runtime_error(
                    &node->operator->ls,
                    "cannot apply '%s' to arguments type 'string'",
                    node->operator->lexeme
                );
            }

            default: break;
        }
    }
    else {
        runtime_error(
            &node->operator->ls,
            "cannot apply binary operator to operands type '%s' and '%s'",
            EpsDbg_GetObjectTypeString(left->type),
            EpsDbg_GetObjectTypeString(right->type)
        );
    }

    Eps_ObjectDestroy(left);
    Eps_ObjectDestroy(right);

    return create_void();
}

static Eps_Object *
visit_unary(Eps_Env *env, Eps_AstUnaryNode* node)
{
    _DEBUG("%*sUNARY\n", 8, "");
    Eps_Object *right = expression(env, node->right);

    if (right->type == OBJ_VOID) {
        return create_void();
    }

    switch (right->type) {
        case OBJ_STRING:
        {
            runtime_error(
                &node->operator->ls,
                "Cannot apply unary operator to operand type '%s'",
                EpsDbg_GetObjectTypeString(right->type)
            );
        } break;
        case OBJ_REAL: break;
        default: break;
    }

    switch (node->operator->toktype) {
        case MINUS:
        {
            if (right->type != OBJ_REAL) {
                runtime_error(
                    &node->operator->ls,
                    "cannot apply %s to expression type %s",
                    node->operator->lexeme,
                    EpsDbg_GetObjectTypeString(right->type)
                );

                return create_void();
            }

            return create_number(- *(double *)right->value);
        } break;
        default: break;
    }

    Eps_ObjectDestroy(right);
    return right;
}

static Eps_Object *
visit_call(Eps_Env *env, Eps_AstPrimaryNode *node)
{
    _DEBUG("%*sPRIMARY\n", 12, "");
    Eps_StatementFunc *func = Eps_EnvGet(env, node->func->identifier->lexeme);

    if (func == NULL) { // if function is not defined
        runtime_error(
            &node->func->identifier->ls,
            "call undefined function '%s'",
            node->func->identifier->lexeme
        );
    }

    Eps_Env *func_env = Eps_EnvCreate();
    func_env->scope = SCOPE_FUNC;
    func_env->enclosing = env;

    EpsList_Node *current_arg = node->func->args->head;
    EpsList_Node *current_param = func->params->head;

    while (current_param != NULL) {
        if (current_arg == NULL) { // if we're out of arguments
            runtime_error(
                &node->func->identifier->ls,
                "too few arguments in function '%s' call",
                node->func->identifier->lexeme
            );

            return create_void();
        }

        Eps_EnvDefine(
            func_env,
            ((Eps_Token *)current_param->data)->lexeme,
            expression(env, current_arg->data)
        );

        current_arg = current_arg->next;
        current_param = current_param->next;
    }

    if (current_arg != NULL) { // if there is arguments left
        runtime_error(
            &node->func->identifier->ls,
            "too much argiments in '%s' function call",
            node->func->identifier->lexeme
        );

        return create_void();
    }

    // function return value
    StmtResult *ret = statement(func_env, func->body);
    Eps_Object *val;


    if (ret->type == STMT_RES_RET) {
        val = ret->ret.val;
    } else {
        val = create_void();
    }

    // Eps_EnvDestroy(func_env);

    return val;
}

static Eps_Object *
visit_primary(Eps_Env *env, Eps_AstPrimaryNode *node)
{
    _DEBUG("%*sPRIMARY, type=%u\n", 8, "", node->type);
    switch (node->type) {
        case PRIMARY_LIT:
        {
            return node->literal;
        } break;
        case PRIMARY_PAREN:
        {
            return expression(env, node->expr);
        } break;
        case PRIMARY_CALL:
        {
            _DEBUG("%*sCALL FUNCTION '%s'\n", 12, "", node->func->identifier->lexeme);
            return visit_call(env, node);
        } break;
        case PRIMARY_ID:
        {
            Eps_Object *ref = Eps_EnvGet(
                env,
                node->identifier->lexeme
            );

            if(ref != NULL) {
                return Eps_ObjectClone(ref);
            } else {
                runtime_error(
                    &node->identifier->ls,
                    "reference to undefined name '%s'",
                    node->identifier->lexeme
                );
            }
        } break;
    }

    return create_void();
}

static Eps_Object *
expression(Eps_Env *env, Eps_Expression* expr)
{
    _DEBUG("    EXPRESSION:\n");
    switch (expr->type) {
        case NODE_TERNARY:
            return visit_ternary(env, expr->ternary);

        case NODE_BIN:
            return visit_binary(env, expr->binary);

        case NODE_UNARY:
            return visit_unary(env, expr->unary);

        case NODE_PRIMARY:
            return visit_primary(env, expr->primary);

        default: break;
    }

    return NULL;
}

// * - Executing Statements -

static StmtResult *
visit_expr_stmt(Eps_Env *env, Eps_StatementExpr *stmt)
{
    expression(env, stmt->expr);

    return stmt_res_noop();
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
        res = statement(block_env, node->data);
        node = node->next; // keep moving on

        if (res->type != STMT_RES_NOOP) return res;
    }

    // Eps_EnvDestroy(block_env);

    return stmt_res_noop();
}

static StmtResult *
visit_if(Eps_Env *env, Eps_StatementConditional *stmt)
{
    Eps_Object *cond = expression(env, stmt->cond);

    if (cond->type != OBJ_BOOL) {
        runtime_error(
            &stmt->cond->ls,
            "invalid condition type '%s'",
            EpsDbg_GetObjectTypeString(cond->type)
        );

        Eps_ObjectDestroy(cond);
        return stmt_res_noop();
    }


    if (*(double *)cond->value) {
        return statement(env, stmt->body);
    } else if (stmt->_else != NULL) {
        return statement(env, stmt->_else);
    }

    Eps_ObjectDestroy(cond);
    return stmt_res_noop();
}

static StmtResult *
visit_output(Eps_Env *env, Eps_StatementOutput *stmt)
{
    Eps_Object *val = expression(env, stmt->expr);


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
            runtime_error(
                &stmt->keyword->ls,
                "cannot output value type of 'void'"
            );
    }

    Eps_ObjectDestroy(val);

    return stmt_res_noop();
}

static StmtResult *
visit_return(Eps_Env *env, Eps_StatementReturn *stmt)
{
    return stmt_res_return(expression(env, stmt->expr), stmt);
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
        runtime_error(
            &stmt->identifier->ls,
            "function '%s' is already defined",
            stmt->identifier->lexeme
        );
    }

    return stmt_res_noop();
}

static StmtResult *
visit_const(Eps_Env *env, Eps_StatementVar *stmt)
{
    if (!Eps_EnvGetLocal(env, stmt->identifier->lexeme)) {
        Eps_Object *val = expression(env, stmt->expr);

        // if const type matches value type
        if(val->type == stmt->type) {
            Eps_EnvDefine(
                env,
                stmt->identifier->lexeme,
                val
            );
        } else {
            runtime_error(
                &stmt->identifier->ls,
                "cannot assign value type '%s' to const type '%s'",
                EpsDbg_GetObjectTypeString(val->type),
                EpsDbg_GetObjectTypeString(stmt->type)
            );
        }
    } else {
        runtime_error(
            &stmt->identifier->ls,
            "constant '%s' is already defined",
            stmt->identifier->lexeme
        );
    }

    return stmt_res_noop();
}

static StmtResult *
visit_define(Eps_Env *env, Eps_StatementVar *stmt)
{
    Eps_Object *temp = Eps_EnvGetLocal(env, stmt->identifier->lexeme);

    if (temp == NULL) {
        Eps_Object *val = expression(env, stmt->expr);

        // if variable type matches value type
        if(val->type == stmt->type) {
            Eps_EnvDefine(
                env,
                stmt->identifier->lexeme,
                val
            );
        } else {
            runtime_error(
                &stmt->identifier->ls,
                "cannot assign value type '%s' to variable type '%s'",
                EpsDbg_GetObjectTypeString(val->type),
                EpsDbg_GetObjectTypeString(stmt->type)
            );
        }
    } else {
        runtime_error(
            &stmt->identifier->ls,
            "varable '%s' is already defined",
            stmt->identifier->lexeme
        );
    }

    return stmt_res_noop();
}

static StmtResult *
visit_assign(Eps_Env *env, Eps_StatementVar *stmt)
{
    Eps_Object *ref_val = Eps_EnvGet(env, stmt->identifier->lexeme);
    Eps_Object *new_val = expression(env, stmt->expr);
    bool is_impicit = ref_val == NULL;

    // check if implicit declaration
    if (is_impicit) {
        runtime_error(
            &stmt->identifier->ls,
            "varable '%s' is not defined",
            stmt->identifier->lexeme
        );

        return stmt_res_noop();
    }

    // check if types matches
    if (ref_val->type != new_val->type) {
        runtime_error(
            &stmt->identifier->ls,
            "cannot assign '%s' to variable type '%s'",
            EpsDbg_GetObjectTypeString(new_val->type),
            EpsDbg_GetObjectTypeString(ref_val->type)
        );

        return stmt_res_noop();
    }

    // if reference value is not mutable
    if(ref_val->mut == false) {
        runtime_error(
            &stmt->identifier->ls,
            "cannot assign value to const '%s'",
            stmt->identifier->lexeme
        );

        return stmt_res_noop();
    }

    Eps_ObjectDestroy(ref_val);
    *ref_val = *new_val;

    return stmt_res_noop();
}

static StmtResult *
statement(Eps_Env *env, Eps_Statement *stmt)
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

    return stmt_res_noop();
}

void
Eps_Interpret(EpsList *stmts)
{
    _DEBUG("--------------- INTERPRETER ---------------\n");

    EpsList_Node *stmt = stmts->head;
    Eps_Env *env = Eps_EnvCreate();

    while (!EpsErr_WasError() && stmt != NULL) {
        // Free statement result
        Eps_FreeMem(
            statement(env, (Eps_Statement *)stmt->data)
        );
        stmt = stmt->next;
    }
}
