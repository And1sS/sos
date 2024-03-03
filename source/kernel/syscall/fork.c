#include "../lib/types.h"
#include "../threading/process/process.h"
#include "../threading/process/uprocess.h"

u64 sys_fork(struct cpu_context* context) { return uprocess_fork(context); }
