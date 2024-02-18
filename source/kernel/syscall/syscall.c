#include "syscall.h"
#include "../scheduler/scheduler.h"

typedef u64 syscall0_handler(struct cpu_context* context);

typedef u64 syscall1_handler(u64 arg0, struct cpu_context* context);

typedef u64 syscall2_handler(u64 arg0, u64 arg1, struct cpu_context* context);

typedef u64 syscall3_handler(u64 arg0, u64 arg1, u64 arg2,
                             struct cpu_context* context);

typedef u64 syscall4_handler(u64 arg0, u64 arg1, u64 arg2, u64 arg3,
                             struct cpu_context* context);

typedef u64 syscall5_handler(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
                             struct cpu_context* context);

typedef u64 syscall6_handler(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
                             u64 arg5, struct cpu_context* context);

typedef struct {
    void* handler;
    u8 arguments_count;
} syscall_descriptor;

#define SYSCALLN(impl, n)                                                      \
    { .handler = impl, .arguments_count = n }

#define SYSCALL0(handler) SYSCALLN(handler, 0)
#define SYSCALL1(handler) SYSCALLN(handler, 1)
#define SYSCALL2(handler) SYSCALLN(handler, 2)
#define SYSCALL3(handler) SYSCALLN(handler, 3)
#define SYSCALL4(handler) SYSCALLN(handler, 4)
#define SYSCALL5(handler) SYSCALLN(handler, 5)
#define SYSCALL6(handler) SYSCALLN(handler, 6)

static syscall_descriptor syscall_handlers[1024] = {
    [SYS_PRINT] = SYSCALL1(sys_print),
    [SYS_PRINT_U64] = SYSCALL2(sys_print_u64),
    [SYS_SIGACTION] = SYSCALL2(sys_set_sigaction),
    [SYS_SIGRET] = SYSCALL0(sys_sigret),
    [SYS_EXIT] = SYSCALL1(sys_exit),
    [SYS_PTHREAD_RUN] = SYSCALL3(sys_pthread_run),
    [SYS_PTHREAD_DETACH] = SYSCALL1(sys_pthread_detach),
    [SYS_PTHREAD_JOIN] = SYSCALL2(sys_pthread_join),

    [SYSCALLS_IMPLEMENTED_COUNT + 1 ... SYSCALLS_MAX_COUNT - 1] = {0}};

u64 handle_syscall(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5,
                   u64 syscall_number, struct cpu_context* context) {

    if (syscall_number >= SYSCALLS_MAX_COUNT)
        return fallback_syscall_handler(arg0, arg0, arg2, arg3, arg4, arg5,
                                        syscall_number, context);

    syscall_descriptor descriptor = syscall_handlers[(u16) syscall_number];
    if (!descriptor.handler)
        return fallback_syscall_handler(arg0, arg1, arg2, arg3, arg4, arg5,
                                        syscall_number, context);

    switch (descriptor.arguments_count) {
    case 0:
        return ((syscall0_handler*) descriptor.handler)(context);
    case 1:
        return ((syscall1_handler*) descriptor.handler)(arg0, context);
    case 2:
        return ((syscall2_handler*) descriptor.handler)(arg0, arg1, context);
    case 3:
        return ((syscall3_handler*) descriptor.handler)(arg0, arg1, arg2,
                                                        context);
    case 4:
        return ((syscall4_handler*) descriptor.handler)(arg0, arg1, arg2, arg3,
                                                        context);
    case 5:
        return ((syscall5_handler*) descriptor.handler)(arg0, arg1, arg2, arg3,
                                                        arg4, context);
    case 6:
        return ((syscall6_handler*) descriptor.handler)(arg0, arg1, arg2, arg3,
                                                        arg4, arg5, context);
    default:
        panic("Invalid arguments count of syscall handler");
    }

    __builtin_unreachable();
}