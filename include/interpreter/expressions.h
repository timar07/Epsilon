#ifndef _EXPRESSIONS_H
#   define _EXPRESSIONS_H

#include "interpreter/enviroment.h"
#include "parser.h"
#include "core/object.h"

Eps_Object *
Eps_EvalExpr(Eps_Env *env, Eps_Expression* expr);

#endif
