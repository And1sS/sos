#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#include "../linked_list/linked_list.h"

#define queue linked_list
#define queue_entry linked_list_node

#define queue_create linked_list_create
#define queue_entry_create linked_list_node_create

#define queue_init linked_list_init
#define queue_push linked_list_add_last
#define queue_pop linked_list_remove_first

#endif // OS_QUEUE_H
