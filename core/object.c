#include "core/object.h"
#include "core/memory.h"
#include <string.h>
#include <stdio.h>

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

static Eps_Object *
bool_to_string(Eps_Object *obj)
{
    return EpsObject_Create(
        OBJ_STRING,
        strdup(*(bool *)obj->value ? "true": "false"),
        obj->mut
    );
}

static Eps_Object *
real_to_string(Eps_Object *obj)
{
    size_t len;
    char *str;

    len = (size_t)snprintf(NULL, 0, "%f", *(double *)obj->value) + 1;
    str = EpsMem_Alloc(len);
    snprintf(str, len, "%g", *(double *)obj->value);

    return EpsObject_Create(OBJ_STRING, str, obj->mut);
}

Eps_Object *
EpsObject_ToString(Eps_Object *obj)
{
    switch (obj->type) {
        case OBJ_REAL:
            return real_to_string(obj);
        case OBJ_BOOL:
            return bool_to_string(obj);
        case OBJ_STRING: // nothing to do, just clone
            return EpsObject_Clone(obj);
        default: return NULL;
    }
}
