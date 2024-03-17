#ifndef SOS_UTHREAD_H
#define SOS_UTHREAD_H

#include "../arch/common/vmm.h"
#include "thread.h"

#define USER_STACK_SIZE 8192
#define USER_STACK_PAGE_COUNT (USER_STACK_SIZE / PAGE_SIZE)

typedef u64 uthread_func();

#define uthread thread

uthread* uthread_create(string name, uthread_func* func);
uthread* uthread_run(string name, void* stack, uthread_func* func);

void uthread_start(string name, void* stack, uthread_func* func);

// should be called when creating first thread
uthread* uthread_create_orphan(process* proc, string name, void* user_stack,
                               uthread_func* func);

#endif // SOS_UTHREAD_H
