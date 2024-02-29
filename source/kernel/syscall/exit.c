#include "../lib/types.h"
#include "../lib/util.h"
#include "../threading/process.h"

struct cpu_context;

_Noreturn u64 sys_exit(u64 arg0, struct cpu_context* context) {
    UNUSED(context);

    process_exit(arg0);

    __builtin_unreachable();
}
