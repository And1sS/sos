#ifndef SOS_ARRAY_LIST_H
#define SOS_ARRAY_LIST_H

#include "../../types.h"

#define INITIAL_CAPACITY 8

typedef struct {
    void** array;
    u64 capacity;
    u64 size;
} array_list;

array_list* array_list_create(u64 capacity);
void array_list_init(array_list* list, u64 capacity);

void array_list_clear(array_list* list);

void* array_list_get(array_list* list, u64 index);
bool array_list_set(array_list* list, u64 index, void* value);

void array_list_add_first(array_list* list, void* value);
void array_list_add_last(array_list* list, void* value);

void* array_list_remove_first(array_list* list);
void* array_list_remove_last(array_list* list);

#endif // SOS_ARRAY_LIST_H
