#include "wait.h"
#include "syscall.h"

long long wait(long long* exit_code) {
    return syscall1(SYS_WAIT, (long long) exit_code);
}