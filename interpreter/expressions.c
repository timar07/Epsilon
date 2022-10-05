#include "interpreter/expressions.h"
#include "interpreter/enviroment.h"
#include "interpreter/statements.h"
#include "interpreter/runtime_errors.h"
#include "parser.h"
#include "ast.h"
#include "core/debug_macros.h"
#include "core/object.h"
#include "core/errors.h"
#include "core/memory.h"
#include <string.h>

// *  - Utils -
static Eps_Object *
create_number(double val)
{
    double *obj_val = EpsMem_Alloc(sizeof(double));
    *obj_val = val;

    return EpsObject_Create(OBJ_REAL, obj_val, true);
}

static Eps_Object *
create_boolean(bool val)
{
    double *obj_val = EpsMem_Alloc(sizeof(bool));
    *obj_val = val;

    return EpsObject_Create(OBJ_BOOL, obj_val, true);
}

static Eps_Object *
create_string(char *str)
{
    return EpsObject_Create(OBJ_STRING, str, true);
}

static Eps_Object *
create_void()
{
    return EpsObject_Create(OBJ_VOID, NULL, true);
}

static Eps_Object *
concat_strings(Eps_Object *left, Eps_Object *right)
{
    size_t len = 80;
    char *buff = EpsMem_Alloc(sizeof(char)*len);

    sprintf(
        buff,
        "%s%s",
        (char *)left->value,
        (char *)right->value
    );

    return create_string(buff);
}

// * - Evaluating Expressions -
Eps_Object *
Eps_EvalExpr(Eps_Env *env, Eps_Expression* expr);

static Eps_Object *
visit_ternary(Eps_Env *env, Eps_AstTernaryNode* node)
{
    _DEBUG("%*sTERNARY\n", 8, "");
    Eps_Object *cond = Eps_EvalExpr(env, node->cond);

    if (cond->type != OBJ_BOOL) {
        // TODO: Throw runtime error
        return create_void();
    }

    if (*(double *)cond->value) {
        return Eps_EvalExpr(env, node->left);
    } else {
        return Eps_EvalExpr(env, node->right);
    }
}

static Eps_Object *
visit_binary(Eps_Env *env, Eps_AstBinNode* node)
{
    _DEBUG("%*sBINARY %s\n", 8, "",
        _EpsDbg_GetTokenTypeString(node->operator->toktype));

    Eps_Object *left = Eps_EvalExpr(env, node->left);
    Eps_Object *right = Eps_EvalExpr(env, node->right);

    if (left->type == OBJ_VOID || right->type == OBJ_VOID) {
        return create_void();
    }

    if (left->type == OBJ_REAL && right->type == OBJ_REAL) {
        double lval = *(double *)left->value;
        double rval = *(double *)right->value;

        switch (node->operator->toktype) {
            case PLUS:
                return create_number(lval + rval);

            case MINUS:
                return create_number(lval - rval);

            case STAR:
                return create_number(lval * rval);

            case SLASH:
                return create_number(lval / rval);

            case EQUAL:
                return create_boolean(lval == rval);

            case BANG_EQUAL:
                return create_boolean(lval != rval);

            case LESS_EQUAL:
                return create_boolean(lval <= rval);

            case GREATER_EQUAL:
                return create_boolean(lval >= rval);

            case LESS:
                return create_boolean(lval < rval);

            case GREATER:
                return create_boolean(lval > rval);

            default: break;
        }
    }
    else if (left->type == OBJ_STRING && right->type == OBJ_STRING) {
        switch (node->operator->toktype) {
            case PLUS:
                return concat_strings(left, right);
            default:
            {
                EpsErr_RuntimeError(
                    &node->operator->ls,
                    "cannot apply '%s' to arguments type 'string'",
                    node->operator->lexeme
                );
            }
        }
    }
    else {
        EpsErr_RuntimeError(
            &node->operator->ls,
            "cannot apply binary operator to operands type '%s' and '%s'",
            EpsDbg_GetObjectTypeString(left->type),
            EpsDbg_GetObjectTypeString(right->type)
        );
    }

    EpsObject_Destroy(left);
    EpsObject_Destroy(right);

    return create_void();
}

static Eps_Object *
visit_unary(Eps_Env *env, Eps_AstUnaryNode* node)
{
    _DEBUG("%*sUNARY\n", 8, "");
    Eps_Object *right = Eps_EvalExpr(env, node->right);

    if (right->type == OBJ_VOID) {
        return create_void();
    }

    switch (node->operator->toktype) {
        case MINUS:
        {
            if (right->type != OBJ_REAL) {
                EpsErr_RuntimeError(
                    &node->operator->ls,
                    "cannot apply %s to expression type %s",
                    node->operator->lexeme,
                    EpsDbg_GetObjectTypeString(right->type)
                );

                return create_void();
            }

            return create_number(- *(double *)right->value);
        } break;
        case STR:
            return EpsObject_ToString(right);
        default:
        {
            EpsErr_RuntimeError(
                &node->operator->ls,
                "unknown operator '%s'",
                node->operator->lexeme
            );
        };
    }

    EpsObject_Destroy(right);
    return right;
}

static Eps_Object *
visit_call(Eps_Env *env, Eps_AstPrimaryNode *node)
{
    _DEBUG("%*sPRIMARY\n", 12, "");
    Eps_StatementFunc *func = Eps_EnvGet(env, node->func->identifier->lexeme);

    if (func == NULL) { // if function is not defined
        EpsErr_RuntimeError(
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
            EpsErr_RuntimeError(
                &node->func->identifier->ls,
                "too few arguments in function '%s' call",
                node->func->identifier->lexeme
            );

            return create_void();
        }

        Eps_EnvDefine(
            func_env,
            ((Eps_Token *)current_param->data)->lexeme,
            Eps_EvalExpr(env, current_arg->data)
        );

        current_arg = current_arg->next;
        current_param = current_param->next;
    }

    if (current_arg != NULL) { // if there is arguments left
        EpsErr_RuntimeError(
            &node->func->identifier->ls,
            "too much argiments in '%s' function call",
            node->func->identifier->lexeme
        );

        return create_void();
    }

    StmtResult *stmt_res = Eps_RunStatement(func_env, func->body);
    Eps_Object *val;

    // if function returned value
    if (stmt_res && stmt_res->type == STMT_RES_RET) {
        val = stmt_res->ret.val;

        if (val->type != func->type) {
            EpsErr_RuntimeError(
                &stmt_res->ret.stmt->expr->ls,
                "cannot return '%s' from a function type '%s'",
                EpsDbg_GetObjectTypeString(val->type),
                EpsDbg_GetObjectTypeString(func->type)
            );
        }
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
            return Eps_EvalExpr(env, node->expr);
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
                return EpsObject_Clone(ref);
            } else {
                EpsErr_RuntimeError(
                    &node->identifier->ls,
                    "reference to undefined name '%s'",
                    node->identifier->lexeme
                );
            }
        } break;
    }

    return create_void();
}

Eps_Object *
Eps_EvalExpr(Eps_Env *env, Eps_Expression* expr)
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
