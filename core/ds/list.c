#include "core/ds/list.h"
#include "lexer.h"
#include "core/memory.h"
#include <stdio.h>

static EpsList_Node *
create_node(EpsList_Node *prev, EpsList_Node *next, void *data)
{
    EpsList_Node *node = Eps_AllocMem(sizeof(EpsList_Node));

    node->prev = prev;
    node->next = next;
    node->data = data;

    return node;
}

EpsList *
EpsList_Create(void)
{
    EpsList *list = Eps_AllocMem(sizeof(EpsList));
    list->head = NULL;
    list->last = NULL;

    return list;
}

void
EpsList_Destroy(EpsList *list, void (*callback)(void *data))
{
    EpsList_Node *tmp;
    EpsList_Node *current = list->head;

    while (current->next != NULL) {
        tmp = current;
        (*callback)(current->data);
        current = tmp->next;
    }

    Eps_FreeMem(list);
}

void
EpsList_Append(EpsList *list, void *data)
{
    EpsList_Node *node;

    if (list->head == NULL) {
        node = create_node(NULL, NULL, data);
        list->head = node;
        list->last = node;
    } else {
        node = create_node(list->last, NULL, data);
        list->last->next = node;
        list->last = node;
    }
}

void *
EpsList_Pop(EpsList *list)
{
    EpsList_Node *popped = list->last;
    void *data = popped->data;

    list->last = list->last->prev;
    Eps_FreeMem(list->last);

    return data;
}
