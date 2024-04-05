#include "../error/errno.h"
#include "../lib/util.h"
#include "../memory/virtual/umem.h"
#include "../threading/thread.h"
#include "../threading/uthread.h"
#include "syscall.h"

u64 sys_pthread_exit(u64 arg0, struct cpu_context* context) {
    UNUSED(context);

    thread_exit(arg0);
}

u64 sys_pthread_run(u64 arg0, u64 arg1, u64 arg2, struct cpu_context* context) {
    UNUSED(context);

    u64 return_code = 0;

    uthread* created = uthread_create((string) arg0, (uthread_func*) arg1);
    if (!created)
        return -ENOMEM;

    if (!copy_to_user((void*) arg2, (void*) &created, sizeof(u64))) {
        thread_signal(created, SIGKILL);
        return_code = -EFAULT;
    }

    thread_start(created);

    return return_code;
}

// TODO: these are unsafe
u64 sys_pthread_detach(u64 arg0, struct cpu_context* context) {
    UNUSED(context);

    thread_detach((uthread*) arg0);

    return 0;
}

u64 sys_pthread_join(u64 arg0, u64 arg1, struct cpu_context* context) {
    UNUSED(context);

    thread_join((uthread*) arg0, (u64*) arg1);

    return 0;
}
