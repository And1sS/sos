#include "syscall.h"

long long getpid() { return syscall0(SYS_GETPID); }