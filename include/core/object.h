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
EpsObject_Create(Eps_ObjectType type, void *val, bool mut);

Eps_Object *
EpsObject_Clone(Eps_Object *obj);

const char *
EpsDbg_GetObjectTypeString(Eps_ObjectType obj_type);

void
EpsObject_Destroy(Eps_Object *obj);

// Converts object to string.
// Note: if OBJ_STRING passed, returns clone of this object.
Eps_Object *
EpsObject_ToString(Eps_Object *obj);

#endif
