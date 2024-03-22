#include <stdlib.h>
#include "nodes.h"


void nodes_pass(void *data){}

struct nodes *nodes_new()
{
    struct nodes* new_nodes;
    new_nodes = malloc(sizeof(struct nodes));
    new_nodes->head = NULL;
    new_nodes->tail = NULL;
    new_nodes->length = 0;
    return new_nodes;
}

int nodes_length(struct nodes *nds)
{
    return nds->length;
}

void nodes_push_back(struct nodes *nds, void *data)
{
    struct nodes_node *n;
    n = malloc(sizeof(struct nodes_node));
    n->data = data;
    n->next = NULL;
    n->prev = NULL;

    if(nds->length == 0){
        nds->head = n;
        nds->tail = n;
    }
    else{
        nds->tail->next = n;
        n->prev = nds->tail;
        nds->tail = n;
    }
    nds->length++;
}

void nodes_push_front(struct nodes *nds, void *data)
{
    struct nodes_node *n;
    n = malloc(sizeof(struct nodes_node));
    n->data = data;
    n->next = NULL;
    n->prev = NULL;

    if(nds->length == 0){
        nds->head = n;
        nds->tail = n;
    }
    else{
        nds->head->prev = n;
        n->next = nds->head;
        nds->head = n;
    }
    nds->length++;
}

void nodes_insert(struct nodes *nds, int index, void *data)
{
    struct nodes_node *n, *tmp;

    if(index < 0 || index > nds->length){
        return;
    }

    if(index == 0){
        nodes_push_front(nds, data);
        return;
    }
    else if(index == nds->length){
        nodes_push_back(nds, data);
        return;
    }

    n = malloc(sizeof(struct nodes_node));
    n->data = data;
    n->next = NULL;
    n->prev = NULL;
    tmp = nds->head;

    for(; index > 0; index--, tmp = tmp->next){}

    n->next = tmp;
    n->prev = tmp->prev;
    tmp->prev->next = n;
    tmp->prev = n;

    nds->length++;
}

void nodes_remove_back(struct nodes *nds, void (*func)(void *))
{
    struct nodes_node *tmp;

    if(nds->length == 0){
        return;
    }
    tmp = nds->tail;
    if(nds->length == 1){
        nds->tail = NULL;
        nds->head = NULL;
    }
    else{
        nds->tail = nds->tail->prev;
        nds->tail->next = NULL;
    }
    func(tmp->data);
    free(tmp);
    nds->length--;
}

void nodes_remove_front(struct nodes *nds, void (*func)(void *))
{
    struct nodes_node *tmp;

    if(nds->length == 0){
        return;
    }
    tmp = nds->head;
    if(nds->length == 1){
        nds->tail = NULL;
        nds->head = NULL;
    }
    else{
        nds->head = nds->head->next;
        nds->head->prev = NULL;
    }
    func(tmp->data);
    free(tmp);
    nds->length--;
}

void nodes_remove(struct nodes *nds, int index, void (*func)(void *))
{
    struct nodes_node *tmp;

    if(index < 0 || index > nds->length - 1){
        return;
    }

    if(index == 0){
        nodes_remove_front(nds, func);
        return;
    }
    else if(index == nds->length - 1){
        nodes_remove_back(nds, func);
        return;
    }

    tmp = nds->head;

    for(; index > 0; index--, tmp = tmp->next){}

    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;

    func(tmp->data);
    free(tmp);
    nds->length--;
}

void *nodes_get_back(struct nodes *nds)
{
    if(!nds->tail){
        return NULL;
    }
    return nds->tail->data;
}

void *nodes_get_front(struct nodes *nds)
{
    if(!nds->head){
        return NULL;
    }
    return nds->head->data;
}

void *nodes_get(struct nodes *nds, int index)
{
    struct nodes_node *tmp;

    if(index < 0 || index > nds->length - 1){
        return NULL;
    }

    if(index == 0){
        return nodes_get_front(nds);
    }
    else if(index == nds->length - 1){
        return nodes_get_back(nds);
    }

    tmp = nds->head;
    for(; index > 0; index--, tmp = tmp->next){}

    return tmp->data;
}

void *nodes_pop(struct nodes *nds)
{
    void *data;
    if(nds->length == 0){
        return NULL;
    }
    data = nodes_get_back(nds);
    nodes_remove_back(nds, nodes_pass);
    return data;
}

void nodes_set_back(struct nodes *nds, void *data)
{
    if(!nds->tail){
        return;
    }
    nds->tail->data = data;
}

void nodes_set_front(struct nodes *nds, void *data)
{
    if(!nds->head){
        return;
    }
    nds->head->data = data;
}

void nodes_set(struct nodes *nds, int index, void *data)
{
    struct nodes_node *tmp;

    if(index < 0 || index > nds->length - 1){
        return;
    }

    if(index == 0){
        return nodes_set_front(nds, data);
    }
    else if(index == nds->length - 1){
        return nodes_set_back(nds, data);
    }

    tmp = nds->head;
    for(; index > 0; index--, tmp = tmp->next){}

    tmp->data = data;
}

void nodes_iter(struct nodes *nds, void *(*func)(void *))
{
    struct nodes_node *tmp;
    tmp = nds->head;
    while(tmp){
        tmp->data = func(tmp->data);
        tmp = tmp->next;
    }
}


void nodes_free(struct nodes *nds, void (*func)(void *))
{
    struct nodes_node *tmp;
    while(nds->head){
        tmp = nds->head;
        func(tmp->data);
        nds->head = nds->head->next;
        free(tmp);
    }
    nds->tail = NULL;
    nds->length = 0;
    free(nds);
}

struct nodes *nodes_slice(struct nodes *nds, int start, int stop)
{
    struct nodes *n;
    int i;
    n = nodes_new(); 
    for(i = start; i < stop; i++){
        nodes_push_back(n, nodes_get(nds, i));
    }
    return n;
}



