#ifndef SOS_SYSCALL_H
#define SOS_SYSCALL_H

#include "../lib/types.h"
#include "../memory/virtual/umem.h"

#define SYS_PRINT 0
#define SYS_PRINT_U64 1

#define SYS_SIGACTION 2
#define SYS_SIGRET 3

#define SYS_PTHREAD_EXIT 4
#define SYS_PTHREAD_RUN 5
#define SYS_PTHREAD_DETACH 6
#define SYS_PTHREAD_JOIN 7

#define SYS_EXIT 8

#define SYS_FORK 9
#define SYS_WAIT 10
#define SYS_GETPID 11

#define SYS_OPEN 12
#define SYS_OPENAT 13
#define SYS_CLOSE 14
#define SYS_READ 15
#define SYS_WRITE 16

#define SYSCALLS_IMPLEMENTED_COUNT 17
#define SYSCALLS_MAX_COUNT 1024

struct cpu_context;

u64 handle_syscall(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5,
                   u64 syscall_number, struct cpu_context* context);

u64 fallback_syscall_handler(u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
                             u64 arg5, u64 syscall_number,
                             struct cpu_context* context);

u64 sys_print(u64 arg0, struct cpu_context* context);
u64 sys_print_u64(u64 arg0, struct cpu_context* context);

u64 sys_set_sigaction(u64 arg0, u64 arg1, struct cpu_context* context);
u64 sys_sigret(struct cpu_context* context);

_Noreturn u64 sys_pthread_exit(u64 arg0, struct cpu_context* context);
u64 sys_pthread_run(u64 arg0, u64 arg1, u64 arg2, struct cpu_context* context);
u64 sys_pthread_detach(u64 arg0, struct cpu_context* context);
u64 sys_pthread_join(u64 arg0, u64 arg1, struct cpu_context* context);

_Noreturn u64 sys_exit(u64 arg0, struct cpu_context* context);

u64 sys_fork(struct cpu_context* context);
u64 sys_wait(u64 arg0, u64 arg1, struct cpu_context* context);
u64 sys_getpid(struct cpu_context* context);

u64 sys_open(string __user path, u64 flags);
u64 sys_openat(u64 fd, string __user path, u64 flags);

u64 sys_close(u64 fd);

u64 sys_read(u64 fd, void* __user buf, u64 size);
u64 sys_write(u64 fd, void* __user buf, u64 size);

#endif // SOS_SYSCALL_H
