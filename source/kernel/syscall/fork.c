#include "../lib/types.h"
#include "../threading/process.h"

u64 sys_fork(struct cpu_context* context) { return process_fork(context); }