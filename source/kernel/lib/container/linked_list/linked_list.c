#include "linked_list.h"
#include "../../../memory/heap/kheap.h"

linked_list* linked_list_create() {
    linked_list* list = (linked_list*) kmalloc(sizeof(linked_list));
    if (!list) {
        return list;
    }

    linked_list_init(list);
    return list;
}

void linked_list_init(linked_list* list) {
    memset(list, 0, sizeof(linked_list));
}

void* linked_list_first(linked_list* list) {
    linked_list_node* node = list->head;
    return node ? node->value : NULL;
}

bool linked_list_add_first(linked_list* list, void* value) {
    linked_list_node* node = linked_list_node_create(value);
    if (!node) {
        return false;
    }

    linked_list_add_first_node(list, node);
    return true;
}

bool linked_list_add_last(linked_list* list, void* value) {
    linked_list_node* node = linked_list_node_create(value);
    if (!node) {
        return false;
    }

    linked_list_add_last_node(list, node);
    return true;
}

void* linked_list_remove_first(linked_list* list) {
    linked_list_node* node = linked_list_remove_first_node(list);
    void* value = NULL;
    if (node) {
        value = node->value;
        kfree(node);
    }

    return value;
}

void* linked_list_remove_last(linked_list* list) {
    linked_list_node* node = linked_list_remove_last_node(list);
    void* value = NULL;
    if (node) {
        value = node->value;
        kfree(node);
    }

    return value;
}

linked_list_node* linked_list_node_create(void* value) {
    linked_list_node* node =
        (linked_list_node*) kmalloc(sizeof(linked_list_node));

    if (!node) {
        return node;
    }

    memset(node, 0, sizeof(linked_list_node));
    node->value = value;
    return node;
}

void linked_list_add_first_node(linked_list* list, linked_list_node* node) {
    node->prev = NULL;
    node->next = NULL;

    if (!list->size) {
        list->head = node;
        list->tail = node;
    } else {
        linked_list_node* old_head = list->head;
        old_head->prev = node;
        node->next = old_head;
        list->head = node;
    }
    list->size++;
}

void linked_list_add_last_node(linked_list* list, linked_list_node* node) {
    node->prev = NULL;
    node->next = NULL;

    if (!list->size) {
        list->head = node;
        list->tail = node;
    } else {
        linked_list_node* old_tail = list->tail;
        old_tail->next = node;
        node->prev = old_tail;
        list->tail = node;
    }
    list->size++;
}

void linked_list_remove_node(linked_list* list, linked_list_node* node) {
    if (list->head == node) {
        linked_list_remove_first_node(list);
    } else if (list->tail == node) {
        linked_list_remove_last_node(list);
    } else {
        linked_list_node* prev = node->prev;
        linked_list_node* next = node->next;

        node->prev = NULL;
        node->next = NULL;

        if (prev) {
            prev->next = next;
        }
        if (next) {
            next->prev = prev;
        }
    }
}

linked_list_node* linked_list_remove_first_node(linked_list* list) {
    if (!list->size) {
        return NULL;
    }

    list->size--;

    linked_list_node* old_head = list->head;
    linked_list_node* old_head_next = old_head->next;

    if (old_head_next) {
        old_head_next->prev = NULL;
        list->head = old_head_next;
    } else {
        list->head = NULL;
        list->tail = NULL;
    }

    old_head->prev = NULL;
    old_head->next = NULL;

    return old_head;
}

linked_list_node* linked_list_remove_last_node(linked_list* list) {
    if (!list->size) {
        return NULL;
    }

    list->size--;

    linked_list_node* old_tail = list->tail;
    linked_list_node* old_tail_prev = old_tail->prev;

    if (old_tail_prev) {
        old_tail_prev->next = NULL;
        list->tail = old_tail_prev;
    } else {
        list->head = NULL;
        list->tail = NULL;
    }

    old_tail->prev = NULL;
    old_tail->next = NULL;

    return old_tail;
}
