#ifndef SOS_UTHREAD_H
#define SOS_UTHREAD_H

#include "thread.h"

typedef void uthread_func();

#define uthread thread

// should be called within user thread that spawns new thread
uthread* uthread_create(string name, void* stack, uthread_func* func);

// should be called when creating first thread
uthread* uthread_create_orphan(process* proc, string name, void* stack,
                               uthread_func* func);

#endif // SOS_UTHREAD_H
