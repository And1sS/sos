#include "../lib/util.h"
#include "../threading/thread.h"
#include "syscall.h"

u64 sys_exit(u64 arg0, struct cpu_context* context) {
    UNUSED(context);

    process_exit(arg0);
}
