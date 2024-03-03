#include "../lib/types.h"
#include "../lib/util.h"
#include "../threading/process/process.h"

u64 sys_wait(struct cpu_context* context) {
    UNUSED(context);

    u64 exit_code;
    return process_wait_any(&exit_code);
}
