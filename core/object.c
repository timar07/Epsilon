#include "core/object.h"
#include "core/memory.h"
#include <string.h>

static const char * obj_strings[] = {
    "real",
    "string",
    "bool",
    "void"
};

Eps_Object *
EpsObject_Create(Eps_ObjectType type, void *val, bool mut)
{
    Eps_Object *obj = EpsMem_Alloc(sizeof(obj));
    obj->type = type;
    obj->value = val;
    obj->mut = mut;

    return obj;
}

Eps_Object *
EpsObject_Clone(Eps_Object *obj)
{
    void *val = NULL;

    switch (obj->type) {
        case OBJ_REAL:
        {
            val = EpsMem_Alloc(sizeof(double));
            *(double *)val = *(double *)obj->value;
        } break;
        case OBJ_STRING:
        {
            val = EpsMem_Alloc(sizeof(char)*strlen(obj->value));
            strcpy(val, obj->value);
        } break;
        default: break;
    }

    return EpsObject_Create(obj->type, val, obj->mut);
}

void
EpsObject_Destroy(Eps_Object *obj)
{
    EpsMem_Free(obj->value);
    EpsMem_Free(obj);
}

const char *
EpsDbg_GetObjectTypeString(Eps_ObjectType obj_type)
{
    return obj_strings[obj_type];
}
