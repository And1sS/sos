#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "../lib/container/linked_list/linked_list.h"
#include "thread.h"

void scheduler_init();

thread* get_current_thread();

void schedule_thread(thread* thrd);

// Should be called at the end of thread execution with interrupts disabled,
// thread lock held and thread->state set to DEAD.
// Will do atomic context switch and release dead thread lock on new context
void schedule_thread_exit();

// Does atomic context switch
void schedule();

struct cpu_context* context_switch(struct cpu_context* context);

void scheduler_lock();
void scheduler_unlock();

#endif // SOS_SCHEDULER_H
