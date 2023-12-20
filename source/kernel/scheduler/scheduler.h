#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "../lib/container/linked_list/linked_list.h"
#include "../threading/thread.h"

void scheduler_init();

thread* get_current_thread();

void schedule_thread(thread* thrd);
void schedule_thread_exit();

struct cpu_context* context_switch(struct cpu_context* context);
void schedule();

void stop_thread(thread* thrd);
void block_thread(thread* thrd);
void unblock_thread(thread* thrd);

#endif // SOS_SCHEDULER_H
