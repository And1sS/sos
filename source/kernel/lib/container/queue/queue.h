#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#include "../linked_list/linked_list.h"

#define queue linked_list
#define queue_node linked_list_node

#define QUEUE_NODE_OF(val) LINKED_LIST_NODE_OF(val)

#define QUEUE_STATIC_INITIALIZER LINKED_LIST_STATIC_INITIALIZER
#define DECLARE_QUEUE(name) DECLARE_LINKED_LIST(name)

#define queue_create linked_list_create

#define queue_init linked_list_init
#define queue_node_create linked_list_node_create
#define queue_push linked_list_add_last_node
#define queue_pop linked_list_remove_first_node
#define queue_remove linked_list_remove_node

#endif // OS_QUEUE_H
