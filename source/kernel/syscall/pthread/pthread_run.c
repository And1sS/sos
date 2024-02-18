#include "../../lib/util.h"
#include "../../threading/uthread.h"

u64 sys_pthread_run(u64 arg0, u64 arg1, u64 arg2, struct cpu_context* context) {
    UNUSED(context);

    uthread* created =
        uthread_create((string) arg0, (void*) 0xF0000, (uthread_func*) arg1);
    thread_start(created);
    *(u64*) arg2 = (u64) created;

    return 0;
}