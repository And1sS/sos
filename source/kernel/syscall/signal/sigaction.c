#include "../../error/errno.h"
#include "../../lib/util.h"
#include "../../memory/virtual/umem.h"
#include "../../scheduler/scheduler.h"
#include "../syscall.h"

u64 sys_set_sigaction(u64 arg0, u64 arg1, struct cpu_context* context) {
    UNUSED(context);

    signal sig = arg0;
    sigaction action;

    if (!copy_from_user(&action, (void*) arg1, sizeof(sigaction)))
        return -EFAULT;

    return thread_set_sigaction(sig, action) ? 0 : -EINVAL;
}