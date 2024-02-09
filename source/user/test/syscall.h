#ifndef SOS_SYSCALL_H
#define SOS_SYSCALL_H

#define SYS_PRINT 0
#define SYS_PRINT_U64 1
#define SYS_SIGACTION 2
#define SYS_SIGRET 3
#define SYS_EXIT 4

long long syscall0(int syscall_number);
long long syscall1(int syscall_number, long long arg0);
long long syscall2(int syscall_number, long long arg0, long long arg1);

#endif // SOS_SYSCALL_H
