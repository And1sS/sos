#include "array_list.h"
#include "../../../memory/heap/kheap.h"
#include "../../memory_util.h"

void array_list_grow(array_list* list);

array_list* array_list_create(u64 capacity) {
    array_list* list = (array_list*) kmalloc(sizeof(array_list));
    if (!list) {
        return NULL;
    }

    array_list_init(list, capacity);
    return list;
}

void array_list_init(array_list* list, u64 capacity) {
    list->array = kmalloc(sizeof(void*) * capacity);
    list->capacity = INITIAL_CAPACITY;
    list->size = 0;
}

void array_list_clear(array_list* list) {
    if (list->capacity > 0) {
        memset(list->array, 0, sizeof(void*) * list->capacity);
    }
}

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

void array_list_add_first(array_list* list, void* value) {
    if (list->size + 1 > list->capacity) {
        array_list_grow(list);
    }

    for (u64 i = list->size; i > 0; i--) {
        list->array[i] = list->array[i - 1];
    }
    list->size++;
    list->array[0] = value;
}

void array_list_add_last(array_list* list, void* value) {
    if (list->size + 1 > list->capacity) {
        array_list_grow(list);
    }

    list->array[list->size++] = value;
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

    return list->array[list->size--];
}

void array_list_grow(array_list* list) {
    u64 new_capacity = list->capacity + list->capacity / 2 + 1;

    list->array = krealloc(list->array, sizeof(void*) * new_capacity);
    list->capacity = new_capacity;
}