#include "../../lib/types.h"
#include "../../lib/util.h"
#include "../../threading/uthread.h"

u64 sys_pthread_detach(u64 arg0, struct cpu_context* context) {
    UNUSED(context);

    thread_detach((uthread*) arg0);

    return 0;
}
