#include "array_list.h"
#include "../../../memory/heap/kheap.h"

void array_list_grow(array_list* list);

array_list* array_list_create() {
    array_list* list = (array_list*) kmalloc(sizeof(array_list));
    array_list_init(list);
    return list;
}

void array_list_init(array_list* list) {
    list->array = kmalloc(sizeof(void*) * INITIAL_CAPACITY);
    list->capacity = INITIAL_CAPACITY;
    list->size = 0;
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