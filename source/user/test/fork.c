#include "fork.h"
#include "syscall.h"

long long fork() { return syscall0(SYS_FORK); }