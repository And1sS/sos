#ifndef OS_LINKED_LIST_H
#define OS_LINKED_LIST_H

#include "../../memory_util.h"
#include "../../types.h"

typedef struct linked_list_node {
    void* value;
    struct linked_list_node* prev;
    struct linked_list_node* next;
} linked_list_node;

typedef struct {
    linked_list_node* head;
    linked_list_node* tail;
    u64 size;
} linked_list;

#define LINKED_LIST_STATIC_INITIALIZER                                         \
    { .head = NULL, .tail = NULL, .size = 0 }

#define DECLARE_LINKED_LIST(name)                                              \
    linked_list name = LINKED_LIST_STATIC_INITIALIZER

linked_list* linked_list_create();
linked_list_node* linked_list_node_create(void* value);

void linked_list_init(linked_list* list);

bool linked_list_add_first(linked_list* list, void* value);
bool linked_list_add_last(linked_list* list, void* value);

void* linked_list_remove_first(linked_list* list);
void* linked_list_remove_last(linked_list* list);

void linked_list_add_first_node(linked_list* list, linked_list_node* node);
void linked_list_add_last_node(linked_list* list, linked_list_node* node);

void linked_list_remove_node(linked_list* list, linked_list_node* node);
linked_list_node* linked_list_remove_first_node(linked_list* list);
linked_list_node* linked_list_remove_last_node(linked_list* list);

#endif // OS_LINKED_LIST_H
