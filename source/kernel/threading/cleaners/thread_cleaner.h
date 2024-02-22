#ifndef SOS_THREAD_CLEANER_H
#define SOS_THREAD_CLEANER_H

#include "../thread.h"

void thread_cleaner_init();

void thread_cleaner_mark(thread* thrd);

#endif // SOS_THREAD_CLEANER_H
