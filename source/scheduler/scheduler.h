#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "thread.h"

void init_scheduler();
void start_scheduler();

void start_thread(thread* thrd);
void switch_context();

void schedule_thread_exit();

void resume_thread(thread* thrd);
void stop_thread(thread* thrd);
void block_thread(thread* thrd);
void unblock_thread(thread* thrd);

#endif // SOS_SCHEDULER_H
