#include "../arch/common/signal.h"
#include "syscall.h"

u64 sys_sigret(struct cpu_context* context) {
    return arch_return_from_signal_handler(context);
}