#ifndef SOS_SYSCALL_H
#define SOS_SYSCALL_H

#define SYS_PRINT 0
#define SYS_PRINT_U64 1

#define SYS_SIGACTION 2
#define SYS_SET_SIGNAL_DISPOSITION 3
#define SYS_SIGRET 4

#define SYS_PTHREAD_EXIT 5
#define SYS_PTHREAD_RUN 6
#define SYS_PTHREAD_DETACH 7
#define SYS_PTHREAD_JOIN 8

#define SYS_EXIT 9

long long syscall0(int syscall_number);
long long syscall1(int syscall_number, long long arg0);
long long syscall2(int syscall_number, long long arg0, long long arg1);
long long syscall3(int syscall_number, long long arg0, long long arg1,
                   long long arg2);

#endif // SOS_SYSCALL_H
