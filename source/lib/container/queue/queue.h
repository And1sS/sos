#ifndef OS_QUEUE_H
#define OS_QUEUE_H

#include "../linked_list/linked_list.h"

#define queue array_list

#define queue_create array_list_create

#define queue_init array_list_init
#define queue_push array_list_add_last
#define queue_pop array_list_remove_first

#endif // OS_QUEUE_H
