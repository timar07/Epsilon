#include "interpreter/enviroment.h"
#include "core/ds/dict.h"
#include "core/memory.h"
#include <stdio.h>

Eps_Env *
Eps_EnvCreate(void)
{
    Eps_Env *env = EpsMem_Alloc(sizeof(Eps_Env));
    env->variables = EpsDict_Create();

    return env;
}

void
Eps_EnvDestroy(Eps_Env *env)
{
    EpsDict_Destroy(env->variables, (void (*)(void *))(&EpsObject_Destroy));
    EpsMem_Free(env);
}

void
Eps_EnvDefine(Eps_Env *env, char *identifier, void *val)
{
    EpsDict_Set(env->variables, identifier, val);
}

void *
Eps_EnvGetLocal(Eps_Env *env, char *identifier)
{
    return EpsDict_Get(env->variables, identifier);
}

void *
Eps_EnvGet(Eps_Env *env, char *identifier)
{
    Eps_Env *current_env = env;
    Eps_Object *val;

    while (current_env != NULL) {
        val = Eps_EnvGetLocal(current_env, identifier);

        if(val != NULL)
            return val;

        current_env = current_env->enclosing;
    }

    return NULL;
}
