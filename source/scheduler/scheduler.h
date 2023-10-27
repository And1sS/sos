#ifndef SOS_SCHEDULER_H
#define SOS_SCHEDULER_H

#include "thread.h"

void init_scheduler();

bool init_thread(thread* thrd, string name, thread_func* func);
void switch_context(thread* thrd);

//void resume_thread(thread* thrd);
void stop_thread(thread* thrd);
void block_thread(thread* thrd);
void unblock_thread(thread* thrd);

#endif // SOS_SCHEDULER_H
