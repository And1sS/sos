#ifndef SOS_SYSCALL_H
#define SOS_SYSCALL_H

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


long long syscall0(int syscall_number);
long long syscall1(int syscall_number, long long arg0);
long long syscall2(int syscall_number, long long arg0, long long arg1);
long long syscall3(int syscall_number, long long arg0, long long arg1,
                   long long arg2);

#endif // SOS_SYSCALL_H