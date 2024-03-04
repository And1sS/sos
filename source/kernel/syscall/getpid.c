#include "../lib/types.h"
#include "../lib/util.h"
#include "../scheduler/scheduler.h"

struct cpu_context;

u64 sys_getpid(struct cpu_context* context) {
    UNUSED(context);

    return process_get_id(get_current_thread()->proc);
}
