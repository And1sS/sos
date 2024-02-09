#include "../error/errno.h"
#include "../lib/kprint.h"
#include "../lib/util.h"
#include "syscall.h"

#define PRINT_ARG(argnum)                                                      \
    do {                                                                       \
        print_u64(argnum);                                                     \
        print(":");                                                            \
        print_u64_hex(arg##argnum);                                            \
        print(", ");                                                           \
    } while (0)

u64 fallback_syscall_handler(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
                             u64 arg5, u64 syscall_number,
                             struct cpu_context* context) {

    UNUSED(context);

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

    return -ENOSYS;
}