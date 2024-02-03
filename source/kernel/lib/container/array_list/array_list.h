#ifndef SOS_ARRAY_LIST_H
#define SOS_ARRAY_LIST_H

#include "../../types.h"

#define ARRAY_LIST_FOR_EACH(list, item)                                        \
    for (u64 ___keep = 1, ___count = 0; ___keep && ___count != (list)->size;   \
         ___keep = !___keep, ___count++)                                       \
        for (item = (list)->array[___count]; ___keep; ___keep = !___keep)

typedef struct {
    void** array;
    u64 capacity;
    u64 size;
} array_list;

array_list* array_list_create(u64 capacity);
void array_list_destroy(array_list* list);

bool array_list_init(array_list* list, u64 capacity);
void array_list_deinit(array_list* list);

void array_list_clear(array_list* list);

void* array_list_get(array_list* list, u64 index);
bool array_list_set(array_list* list, u64 index, void* value);
bool array_list_insert(array_list* list, u64 index, void* value);

bool array_list_add_first(array_list* list, void* value);
bool array_list_add_last(array_list* list, void* value);

void* array_list_remove_first(array_list* list);
void* array_list_remove_last(array_list* list);

bool array_list_remove_idx(array_list* list, u64 idx);
bool array_list_remove(array_list* list, void* value);

#endif // SOS_ARRAY_LIST_H
