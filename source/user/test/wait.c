#include "wait.h"
#include "syscall.h"

long long wait(long long pid, long long* exit_code) {
    return syscall2(SYS_WAIT, pid, (long long) exit_code);
}