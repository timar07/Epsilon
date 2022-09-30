#include "core/ds/dict.h"
#include "core/memory.h"
#include "core/debug_macros.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define INITIAL_CAPACITY 16
#define EPS_DICT_MAX_LOAD 0.75

#define ADJUCT_CAPACITY(dict) Eps_ReallocMem(dict, dict->capacity*2);

struct eps_dict_item_t {
    char *key;
    void *value;
    struct eps_dict_item_t *next;

    // private
    size_t _index;
};

struct eps_dict_t {
    EpsDict_Item **items;
    size_t capacity;
    size_t length;
};

#define FNV_OFFSET 2166136261U
#define FNV_PRIME 16777619U

static uint32_t
hash_key(char *key)
{
    uint32_t hash = FNV_OFFSET;
    for (char *p = key; *p; p++) {
        hash ^= (uint32_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

static size_t
get_index(size_t capacity, char *key)
{
    return hash_key(key) & (capacity - 1);
}

EpsDict *
EpsDict_Create(void)
{
    EpsDict *dict = Eps_AllocMem(sizeof(EpsDict));
    dict->length = 0;
    dict->capacity = INITIAL_CAPACITY;
    dict->items = Eps_CallocMem(sizeof(EpsDict_Item), INITIAL_CAPACITY);

    return dict;
}

static void
destroy_dict_items_chain(EpsDict_Item *item, void (*callback)(void *))
{
    EpsDict_Item *next;
    EpsDict_Item *current = item;

    while (current != NULL) {
        next = current->next;

        // destroying the item
        callback(current->value);
        Eps_FreeMem(current->key);
        Eps_FreeMem(current);

        current = next;
    }
}

void
EpsDict_Destroy(EpsDict *dict, void (*callback)(void *))
{
    size_t i;

    for(i = 0; i < dict->capacity; i++) {
        destroy_dict_items_chain(dict->items[i], callback);
    }

    Eps_FreeMem(dict);
}

void *
EpsDict_Get(EpsDict *dict, char *key)
{
    EpsDict_Item *current = dict->items[get_index(dict->capacity, key)];

    while (current != NULL && current != NULL) {
        if(strcmp(current->key, key) == 0) {
            return current->value;
        }

        current = current->next;
    }

    return NULL;
}

void
EpsDict_Set(EpsDict *dict, char *key, void *val)
{
    EpsDict_Item *item = Eps_AllocMem(sizeof(EpsDict_Item));

    if (dict->length + 1 > dict->capacity * EPS_DICT_MAX_LOAD) {
        ADJUCT_CAPACITY(dict);
    }

    item->_index = get_index(dict->capacity, key);
    item->key = key;
    item->value = val;
    item->next = NULL;

    if (dict->items[item->_index] == NULL) {
        dict->items[item->_index] = item;
    } else {
        EpsDict_Item *current = dict->items[item->_index];

        while (current->next != NULL) {
            current = current->next;
        }

        current->next = item;
    }

    dict->length++;
}

size_t
EpsDict_Length(EpsDict *dict)
{
    return dict->length;
}
