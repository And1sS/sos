#include "../error/errno.h"
#include "../lib/util.h"
#include "../threading/scheduler.h"
#include "syscall.h"

u64 sys_set_sigaction(u64 arg0, u64 arg1, struct cpu_context* context) {
    UNUSED(context);

    return process_set_sigaction(arg0, *(sigaction*) arg1) ? 0 : -EINVAL;
}