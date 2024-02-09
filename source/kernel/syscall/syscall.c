#include "syscall.h"
#include "../lib/kprint.h"
#include "../scheduler/scheduler.h"

typedef struct cpu_context* syscall0_handler(struct cpu_context* context);

typedef struct cpu_context* syscall1_handler(u64 arg0,
                                             struct cpu_context* context);

typedef struct cpu_context* syscall2_handler(u64 arg0, u64 arg1,
                                             struct cpu_context* context);

typedef struct cpu_context* syscall3_handler(u64 arg0, u64 arg1, u64 arg2,
                                             struct cpu_context* context);

typedef struct cpu_context* syscall4_handler(u64 arg0, u64 arg1, u64 arg2,
                                             u64 arg3,
                                             struct cpu_context* context);

typedef struct cpu_context* syscall5_handler(u64 arg0, u64 arg1, u64 arg2,
                                             u64 arg3, u64 arg4,
                                             struct cpu_context* context);

typedef struct cpu_context* syscall6_handler(u64 arg0, u64 arg1, u64 arg2,
                                             u64 arg3, u64 arg4, u64 arg5,
                                             struct cpu_context* context);

typedef struct {
    void* handler;
    u8 arguments_count;
} syscall_descriptor;

static syscall_descriptor syscall_handlers[1024] = {[0 ... 1023] = {0}};

#define PRINT_ARG(argnum)                                                      \
    do {                                                                       \
        print_u64(argnum);                                                     \
        print(":");                                                            \
        print_u64_hex(arg##argnum);                                            \
        print(", ");                                                           \
    } while (0)

struct cpu_context* fallback_syscall_handler(u64 arg0, u64 arg1, u64 arg2,
                                             u64 arg3, u64 arg4, u64 arg5,
                                             u64 syscall_number,
                                             struct cpu_context* context) {

    print("Invalid syscall: ");
    print_u64(syscall_number);
    print(" ");

    PRINT_ARG(0);
    PRINT_ARG(1);
    PRINT_ARG(2);
    PRINT_ARG(3);
    PRINT_ARG(4);
    PRINT_ARG(5);

    println("");

    return context;
}

struct cpu_context* handle_syscall(u64 arg0, u64 arg1, u64 arg2, u64 arg3,
                                   u64 arg4, u64 arg5, u64 syscall_number,
                                   struct cpu_context* context) {

    if (syscall_number > 1024)
        return fallback_syscall_handler(arg0, arg0, arg2, arg3, arg4, arg5,
                                        syscall_number, context);

    syscall_descriptor descriptor = syscall_handlers[(u16) syscall_number];
    if (!descriptor.handler)
        return fallback_syscall_handler(arg0, arg0, arg2, arg3, arg4, arg5,
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