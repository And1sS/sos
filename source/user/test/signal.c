#include "signal.h"
#include "syscall.h"

long process_set_sigaction(signal sig, const sigaction* action) {
    return syscall2(SYS_SIGACTION, (long long) sig, (long long) action);
}