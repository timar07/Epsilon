#ifndef EPS_DICT
#   define EPS_DICT

#include <stddef.h>

typedef struct eps_dict_item_t EpsDict_Item;
typedef struct eps_dict_t EpsDict;

EpsDict *
EpsDict_Create(void);

void
EpsDict_Destroy(EpsDict *dict, void (*callback)(void *));

void *
EpsDict_Get(EpsDict *dict, char *key);

void
EpsDict_Set(EpsDict *dict, char *key, void *val);

size_t
EpsDict_Length(EpsDict *dict);

#endif
