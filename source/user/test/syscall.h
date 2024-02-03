#ifndef SOS_SYSCALL_H
#define SOS_SYSCALL_H

#define SYS_PRINT 0
#define SYS_SIGINSTALL 1
#define SYS_EXIT 2
#define SYS_SIGRET 4

long long syscall0(int syscall_number);
long long syscall1(int syscall_number, long long arg0);
long long syscall2(int syscall_number, long long arg0, long long arg1);

#endif // SOS_SYSCALL_H
