#include "../../error/errno.h"
#include "../../lib/util.h"
#include "../../memory/virtual/umem.h"
#include "../../threading/uthread.h"

u64 sys_pthread_run(u64 arg0, u64 arg1, u64 arg2, struct cpu_context* context) {
    UNUSED(context);

    u64 return_code = 0;

    uthread* created = uthread_create((string) arg0, (uthread_func*) arg1);
    if (!copy_to_user((void*) arg2, (void*) &created, sizeof(u64))) {
        thread_signal(created, SIGKILL);
        return_code = -EFAULT;
    }

    thread_start(created);

    return return_code;
}