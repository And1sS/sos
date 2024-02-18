#include "../../lib/types.h"
#include "../../lib/util.h"
#include "../../threading/uthread.h"

u64 sys_pthread_join(u64 arg0, u64 arg1, struct cpu_context* context) {
    UNUSED(context);

    thread_join((uthread*) arg0, (u64*) arg1);

    return 0;
}