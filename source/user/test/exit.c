#include "exit.h"
#include "syscall.h"

_Noreturn void exit(long long code) {
    syscall1(SYS_EXIT, code);

    __builtin_unreachable();
}