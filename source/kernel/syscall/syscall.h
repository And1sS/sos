#ifndef SOS_SYSCALL_H
#define SOS_SYSCALL_H

#include "../lib/types.h"

struct cpu_context* handle_syscall(u64 arg0, u64 arg1, u64 arg2, u64 arg3,
                                   u64 arg4, u64 arg5, u64 syscall_number,
                                   struct cpu_context* context);

#endif // SOS_SYSCALL_H
