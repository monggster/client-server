#include <stdlib.h>
#include <string.h>
#include "nodes.h"
#include "associations.h"


static int pearson_hash(const char *key, int len) 
{
    int i, j, sum;
    unsigned char h;

    static const unsigned char T[256] = {
    98,  6, 85,150, 36, 23,112,164,135,207,169,  5, 26, 64,165,219,  /* 1*/
    61, 20, 68, 89,130, 63, 52,102, 24,229,132,245, 80,216,195,115,  /* 2*/
    90,168,156,203,177,120,  2,190,188,  7,100,185,174,243,162, 10,  /* 3*/
    123,251, 67,250,161,  0,107, 97,241,111,181, 82,249, 33, 69, 55, /* 4*/
    237, 18,253,225,  8,208,172,244,255,126,101, 79,145,235,228,121, /* 5*/
    59,153, 29,  9,213,167, 84, 93, 30, 46, 94, 75,151,114, 73,222,  /* 6*/
    197, 96,210, 45, 16,227,248,202, 51,152,252,125, 81,206,215,186, /* 7*/
    39,158,178,187,131,136,  1, 49, 50, 17,141, 91, 47,129, 60, 99,  /* 8*/
    154, 35, 86,171,105, 34, 38,200,147, 58, 77,118,173,246, 76,254, /* 9*/
    133,232,196,144,198,124, 53,  4,108, 74,223,234,134,230,157,139, /*10*/
    189,205,199,128,176, 19,211,236,127,192,231, 70,233, 88,146, 44, /*11*/
    183,201, 22, 83, 13,214,116,109,159, 32, 95,226,140,220, 57, 12, /*12*/
    221, 31,209,182,143, 92,149,184,148, 62,113, 65, 37, 27,106,166, /*13*/
    3, 14,204, 72, 21, 41, 56, 66, 28,193, 40,217, 25, 54,179,117,   /*14*/
    238, 87,240,155,180,170,242,212,191,163, 78,218,137,194,175,110, /*15*/
    43,119,224, 71,122,142, 42,160,104, 48,247,103, 15, 11,138,239,  /*16*/
    };

    for (sum = 0, j = 0; j < 8; ++j) {
        h = T[(key[0] + j) % 256];
        for (i = 1; i < len; ++i) {
            h = T[h ^ key[i]];
        }
        sum += h << (7 - j);
    }
    return sum;
}

static int get_index(struct associations *as, char *key)
{
    return pearson_hash(key, strlen(key)) % as->table->length;
}

static struct association *association_new(char *key, void *val)
{
    struct association *a;
    a = malloc(sizeof(struct association));
    a->key = key;
    a->val = val;
    return a;
}

static void associations_resize(struct associations *as)
{
    double coefficient;
    struct nodes *old_table, *new_table, *tmp;
    struct association *a;
    int i, j;

    coefficient = (double)associations_length(as) / (double)(nodes_length(as->table));
    if(coefficient > ASSOCIATIONS_RESIZE_COEF){

        new_table = nodes_new();
        for(i = 0; i < nodes_length(as->table) * 2; i++){
            tmp = nodes_new();
            nodes_push_back(new_table, tmp);
        }
        
        old_table = as->table;
        as->table = new_table;
        for(i = 0; i < nodes_length(old_table); i++){
            tmp = (struct nodes *)nodes_get(old_table, i);
            for(j = 0; j < nodes_length(tmp); j++){
                a = (struct association *)nodes_get(tmp, j);
                associations_set(as, a->key, a->val);
                ////////
                free(a->key);
                //free(a); //первое освобождение 
                ///////
            }
            nodes_free(tmp, free); //второе освобождение
        }
        nodes_free(old_table, nodes_pass);
    }
}

void associations_pass(void *data){}

int associations_length(struct associations *as)
{
    int length, i;
    struct nodes *n, *tmp;

    length = 0;
    n = as->table;
    for(i = 0; i < nodes_length(n); i++){
        tmp = (struct nodes *)nodes_get(n, i);
        length += nodes_length(tmp);
    }
    return length;
}

struct associations *associations_new(int size)
{
    struct associations *as;
    struct nodes *table, *row;;
    int i;

    if(size < ASSOCIATIONS_MINIMAL_SIZE)
        size = ASSOCIATIONS_MINIMAL_SIZE;

    table = nodes_new();
    for(i = 0; i < size; i++){
        row = nodes_new();
        nodes_push_back(table, row);
    }

    as = malloc(sizeof(struct associations));
    as->table = table;

    return as;
}


void associations_set(struct associations *as, char *_key, void *val)
{
    struct association *a;
    struct nodes *n;
    int i, index;
    char *key;

    // creating a copy of the key
    // чтобы если по адресу _key строка поменялась, то хеш ключа
    // оставался валидным

    //key = chars_from(_key);
    key = calloc(1, strlen(_key) + 1);
    memcpy(key, _key, strlen(_key));

    index = get_index(as, key);
    n = (struct nodes *)nodes_get(as->table, index);
    for(i = 0; i < nodes_length(n); i++){
        a = (struct association *)nodes_get(n, i);
        if(!strncmp(a->key, key, strlen(key))){
            a->val = val;
            return;
        }
    }
    a = association_new(key, val);
    nodes_push_back(n, a);
    associations_resize(as);
}

void* associations_get(struct associations *as, char *key)
{
    struct association *a;
    struct nodes *n;
    int i, index;

    index = get_index(as, key);
    n = (struct nodes *)nodes_get(as->table, index);
    for(i = 0; i < nodes_length(n); i++){
        a = (struct association *)nodes_get(n, i);
        if(!strncmp(a->key, key, strlen(key))){
            return a->val;
        }
    }
    return NULL;
}

void associations_remove(struct associations *as, char *key, void (*func)(void *))
{
    struct association *a;
    struct nodes *n;
    int i, index;

    if(associations_get(as, key)){
        index = get_index(as, key);
        n = (struct nodes *)nodes_get(as->table, index);
        for(i = 0; i < nodes_length(n); i++){
            a = (struct association *)nodes_get(n, i);
            if(!strncmp(a->key, key, strlen(key))){
                // удаляем элемент списка
                nodes_remove(n, i, nodes_pass);
                // deleting copy of the key
                free(a->key);
                // using func for val
                func(a->val);
                // deleting key-value pair
                free(a);
            }
        }
    }
}

void associations_iter(struct associations *as, void *(*valfunc)(void *))
{
    struct nodes *n, *tmp;
    struct association *a;
    int i, j;

    n = as->table;
    for(i = 0; i < nodes_length(n); i++){
        tmp = (struct nodes *)nodes_get(n, i);
        for(j = 0; j < nodes_length(tmp); j++){
            a = (struct association *)nodes_get(tmp, j);
            a->val = valfunc(a->val);
        }
    }
}

struct nodes *associations_keys(struct associations *as)
{
    struct nodes *n, *tmp, *keys;
    struct association *a;
    char *key_copy;
    int i, j;

    n = as->table;
    keys = nodes_new();
    for(i = 0; i < nodes_length(n); i++){
        tmp = (struct nodes *)nodes_get(n, i);
        for(j = 0; j < nodes_length(tmp); j++){
            a = (struct association *)nodes_get(tmp, j);
            // making a copy of key 
            // to prevent its changing

            key_copy = calloc(1, strlen(a->key) + 1);
            memcpy(key_copy, a->key, strlen(a->key));
            nodes_push_back(keys, key_copy);
            //nodes_push_back(keys, chars_from(a->key));
        }
    }
    return keys;
}

void associations_free(struct associations *as, void (*func)(void *)) {
    struct nodes *tmp;
    struct association *a;
    int i, j;

    for(i = 0; i < nodes_length(as->table); i++){
        tmp = (struct nodes *)nodes_get(as->table, i);
        for(j = 0; j < nodes_length(tmp); j++){
            a = (struct association *)nodes_get(tmp, j);
            func(a->val);
            free(a->key);
        }
        nodes_free(tmp, free);
    }
    nodes_free(as->table, nodes_pass);
    free(as);
}
