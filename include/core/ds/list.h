#ifndef EPS_LIST
#   define EPS_LIST

typedef struct node {
    void *data;
    struct node *prev;
    struct node *next;
} EpsList_Node;

typedef struct list {
    EpsList_Node *head;
    EpsList_Node *last;
} EpsList;

EpsList *
EpsList_Create(void);

void
EpsList_Destroy(EpsList *list, void (*callback)(void *data));

void
EpsList_Append(EpsList *list, void *value);

void *
EpsList_Pop(EpsList *list);

#endif
