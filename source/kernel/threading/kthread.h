#ifndef SOS_KTHREAD_H
#define SOS_KTHREAD_H

#include "thread.h"

typedef void kthread_func();

#define kthread thread

kthread* kthread_create(string name, kthread_func* func);
kthread* kthread_run(string name, kthread_func* func);

void kthread_start(kthread* thread);
void kthread_kill(kthread* thrd);
bool kthread_should_die();

#endif // SOS_KTHREAD_H
