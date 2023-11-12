#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "../lib/container/linked_list/linked_list.h"
#include "thread.h"

void init_scheduler();

linked_list_node* get_current_thread_node();

void schedule_thread_start(thread* thrd);
void schedule_thread_exit();
void schedule_thread(linked_list_node* thread_node);
void schedule_thread_block();

struct cpu_context* context_switch(struct cpu_context* context);
void schedule();

void stop_thread(thread* thrd);
void block_thread(thread* thrd);
void unblock_thread(thread* thrd);

#endif // SOS_SCHEDULER_H
