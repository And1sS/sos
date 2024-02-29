#include "syscall.h"

long long syscall0(int syscall_number) {
    long long res;

    __asm__ volatile("int $0x80" : "=a"(res) : "a"(syscall_number) : "memory");

    return res;
}

long long syscall1(int syscall_number, long long arg0) {
    long long res;

    register long long reg_arg0 asm("rdi") = arg0;
    __asm__ volatile("int $0x80"
                     : "=a"(res)
                     : "r"(reg_arg0), "a"(syscall_number)
                     : "memory");

    return res;
}

long long syscall2(int syscall_number, long long arg0, long long arg1) {
    long long res;

    register long long reg_arg0 asm("rdi") = arg0;
    register long long reg_arg1 asm("rsi") = arg1;
    __asm__ volatile("int $0x80"
                     : "=a"(res)
                     : "r"(reg_arg0), "r"(reg_arg1), "a"(syscall_number)
                     : "memory");

    return res;
}

long long syscall3(int syscall_number, long long arg0, long long arg1,
                   long long arg2) {

    long long res;

    register long long reg_arg0 asm("rdi") = arg0;
    register long long reg_arg1 asm("rsi") = arg1;
    register long long reg_arg2 asm("rdx") = arg2;
    __asm__ volatile("int $0x80"
                     : "=a"(res)
                     : "r"(reg_arg0), "r"(reg_arg1), "r"(reg_arg2),
                       "a"(syscall_number)
                     : "memory");

    return res;
}
