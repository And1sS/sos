#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "thread.h"

void init_scheduler();

void schedule_thread_start(thread* thrd);
void schedule();

void schedule_thread_exit();

void stop_thread(thread* thrd);
void block_thread(thread* thrd);
void unblock_thread(thread* thrd);

#endif // SOS_SCHEDULER_H
