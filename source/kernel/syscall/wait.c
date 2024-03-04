
#include "../error/errno.h"
#include "../error/error.h"
#include "../lib/types.h"
#include "../lib/util.h"
#include "../memory/virtual/umem.h"
#include "../threading/process.h"

struct cpu_context;

u64 sys_wait(u64 arg0, struct cpu_context* context) {
    UNUSED(context);

    u64 exit_code;

    u64 result = process_wait_any(&exit_code);
    if (IS_ERROR(result))
        return result;

    if (!copy_to_user((void*) arg0, &exit_code, sizeof(u64)))
        return -EFAULT;

    return result;
}