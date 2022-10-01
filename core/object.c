#include "core/object.h"
#include "core/memory.h"

static const char * obj_strings[] = {
    "real",
    "string",
    "bool",
    "void"
};

Eps_Object *
Eps_ObjectCreate(Eps_ObjectType type, void *val, bool mut)
{
    Eps_Object *obj = Eps_AllocMem(sizeof(obj));
    obj->type = type;
    obj->value = val;
    obj->mut = mut;

    return obj;
}

Eps_Object *
Eps_ObjectClone(Eps_Object *obj)
{
    void *val;

    switch (obj->type) {
        case OBJ_REAL:
        {
            val = Eps_AllocMem(sizeof(double));
            *(double *)val = *(double *)obj->value;
        } break;
        case OBJ_STRING:
        {

        } break;
        default: break;
    }

    return Eps_ObjectCreate(obj->type, val, obj->mut);
}

void
Eps_ObjectDestroy(Eps_Object *obj)
{
    Eps_FreeMem(obj->value);
    Eps_FreeMem(obj);
}

const char *
EpsDbg_GetObjectTypeString(Eps_ObjectType obj_type)
{
    return obj_strings[obj_type];
}
