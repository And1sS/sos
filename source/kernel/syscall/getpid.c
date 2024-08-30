#include "../lib/types.h"
#include "../lib/util.h"
#include "../threading/process.h"
#include "../threading/scheduler.h"

struct cpu_context;

u64 sys_getpid(struct cpu_context* context) {
    UNUSED(context);

    return get_current_thread()->proc->id;
}