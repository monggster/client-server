#ifndef NODES_H_SENTRY
#define NODES_H_SENTRY



struct nodes_node
{
    void *data;
    struct nodes_node *next;
    struct nodes_node *prev;
};

struct nodes
{
    int length;
    struct nodes_node *head;
    struct nodes_node *tail;
};

void nodes_pass(void *data);

struct nodes *nodes_new();

int nodes_length(struct nodes *nds);

void nodes_push_back(struct nodes *nds, void *data);

void nodes_push_front(struct nodes *nds, void *data);

void nodes_insert(struct nodes *nds, int index, void *data);

void nodes_remove_back(struct nodes *nds, void (*func)(void *));

void nodes_remove_front(struct nodes *nds, void (*func)(void *));

void nodes_remove(struct nodes *nds, int index, void (*func)(void *));

void *nodes_get_back(struct nodes *nds);

void *nodes_get_front(struct nodes *nds);

void *nodes_get(struct nodes *nds, int index);

void *nodes_pop(struct nodes *nds);

void nodes_set_back(struct nodes *nds, void *data);

void nodes_set_front(struct nodes *nds, void *data);

void nodes_set(struct nodes *nds, int index, void *data);

void nodes_iter(struct nodes *nds, void *(*func)(void *));

void nodes_free(struct nodes *nds, void (*func)(void *));

struct nodes *nodes_slice(struct nodes *nds, int start, int end);

#endif
