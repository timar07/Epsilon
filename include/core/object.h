#ifndef EPS_OBJECT
#   define EPS_OBJECT

#include <stdbool.h>

typedef enum {
    OBJ_REAL,
    OBJ_STRING,
    OBJ_BOOL,
    OBJ_VOID,
} Eps_ObjectType;

typedef struct {
    Eps_ObjectType type;
    void *value;
    bool mut;
} Eps_Object;

Eps_Object *
Eps_ObjectCreate(Eps_ObjectType type, void *val, bool mut);

Eps_Object *
Eps_ObjectClone(Eps_Object *obj);

const char *
EpsDbg_GetObjectTypeString(Eps_ObjectType obj_type);

void
Eps_ObjectDestroy(Eps_Object *obj);

#endif
