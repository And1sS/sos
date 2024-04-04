#include "array_list.h"
#include "../../../memory/heap/kheap.h"
#include "../../memory_util.h"
#include "../../panic.h"

bool array_list_grow(array_list* list);

array_list* array_list_create(u64 capacity) {
    array_list* list = (array_list*) kmalloc(sizeof(array_list));
    if (!list) {
        return NULL;
    }

    if (!array_list_init(list, capacity)) {
        kfree(list);
        return NULL;
    }

    return list;
}

void array_list_destroy(array_list* list) {
    array_list_deinit(list);
    kfree(list);
}

bool array_list_init(array_list* list, u64 capacity) {
    list->array = kmalloc(sizeof(void*) * capacity);
    if (!list->array)
        return false;

    list->capacity = capacity;
    list->size = 0;
    return true;
}

void array_list_clear(array_list* list) {
    if (list->capacity > 0) {
        list->size = 0;
        memset(list->array, 0, sizeof(void*) * list->capacity);
    }
}

void array_list_deinit(array_list* list) { kfree(list->array); }

void* array_list_get(array_list* list, u64 index) {
    return index < list->size ? list->array[index] : NULL;
}

bool array_list_set(array_list* list, u64 index, void* value) {
    if (index >= list->size) {
        return false;
    }

    list->array[index] = value;
    return true;
}

bool array_list_insert(array_list* list, u64 index, void* value) {
    if (list->size + 1 > list->capacity && !array_list_grow(list)) {
        return false;
    }

    for (u64 i = list->size; i > index; i--) {
        list->array[i] = list->array[i - 1];
    }
    list->size++;
    list->array[index] = value;
    return true;
}

bool array_list_add_first(array_list* list, void* value) {
    if (list->size + 1 > list->capacity && !array_list_grow(list)) {
        return false;
    }

    for (u64 i = list->size; i > 0; i--) {
        list->array[i] = list->array[i - 1];
    }
    list->size++;
    list->array[0] = value;
    return true;
}

bool array_list_add_last(array_list* list, void* value) {
    if (list->size + 1 > list->capacity && !array_list_grow(list)) {
        return false;
    }

    list->array[list->size++] = value;
    return true;
}

void* array_list_remove_first(array_list* list) {
    if (list->size == 0) {
        return NULL;
    }

    void* result = list->array[0];
    for (u64 i = 0; i < list->size - 1; i++) {
        list->array[i] = list->array[i + 1];
    }
    list->size--;

    return result;
}

void* array_list_remove_last(array_list* list) {
    if (list->size == 0) {
        return NULL;
    }

    return list->array[list->size-- - 1];
}

bool array_list_remove_idx(array_list* list, u64 idx) {
    if (idx >= list->size) {
        return false;
    }

    for (u64 i = idx; i < list->size - 1; i++) {
        list->array[i] = list->array[i + 1];
    }
    list->size--;
    return true;
}

bool array_list_remove(array_list* list, void* value) {
    for (u64 i = 0; i < list->size; ++i) {
        if (list->array[i] == value) {
            array_list_remove_idx(list, i);
            return true;
        }
    }

    return false;
}

bool array_list_grow(array_list* list) {
    u64 new_capacity = list->capacity + list->capacity / 2 + 1;

    void* new_array = krealloc(list->array, sizeof(void*) * new_capacity);
    if (!new_array)
        return false;

    list->array = new_array;
    list->capacity = new_capacity;
    return true;
}