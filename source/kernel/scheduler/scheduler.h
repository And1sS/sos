#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "../lib/container/linked_list/linked_list.h"
#include "../threading/thread.h"

void scheduler_init();

thread* get_current_thread();

// This function acquires/releases current thread lock, so they should not be
// called with thread`s lock held
void schedule_thread(thread* thrd);

void schedule_thread_exit();

// These functions acquire/release current thread lock, so they should not be
// called with thread`s lock held
struct cpu_context* context_switch(struct cpu_context* context);
void schedule();

#endif // SOS_SCHEDULER_H
