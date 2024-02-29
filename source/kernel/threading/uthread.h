#ifndef SOS_UTHREAD_H
#define SOS_UTHREAD_H

#include "../arch/common/vmm.h"
#include "thread.h"

typedef void uthread_func();

#define uthread thread

#define USER_STACK_SIZE 8192
#define USER_STACK_PAGE_COUNT (USER_STACK_SIZE / PAGE_SIZE)

// should be called within user thread that spawns new thread
uthread* uthread_create(string name, uthread_func* func);

// should be called when creating first thread
uthread* uthread_create_orphan(process* proc, string name, uthread_func* func);

#endif // SOS_UTHREAD_H
