#include "core/ds/dict.h"

typedef enum {
    SCOPE_GLOBAL = 0,
    SCOPE_BLOCK,
    SCOPE_FUNC,
} Eps_EnvScope;

typedef struct eps_env_t {
    Eps_EnvScope scope;
    EpsDict *variables;
    struct eps_env_t *enclosing;
} Eps_Env;

void
Eps_EnvDefine(Eps_Env *env, char *identifier, void *val);

void
Eps_EnvDestroy(Eps_Env *env);

void *
Eps_EnvGetLocal(Eps_Env *env, char *identifier);

void *
Eps_EnvGet(Eps_Env *env, char *identifier);

Eps_Env *
Eps_EnvCreate(void);
