#ifndef SOS_UTHREAD_H
#define SOS_UTHREAD_H

#include "thread.h"

typedef u64 uthread_func();

#define uthread thread

uthread* uthread_create(string name, void* stack, uthread_func* func);
uthread* uthread_run(string name, void* stack, uthread_func* func);

void uthread_start(string name, void* stack, uthread_func* func);

// should be called when creating first thread
uthread* uthread_create_orphan(string name, void* stack, uthread_func* func);

#endif // SOS_UTHREAD_H
