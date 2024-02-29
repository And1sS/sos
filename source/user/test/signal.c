#include "signal.h"
#include "syscall.h"

long pthread_sigaction(signal sig, const sigaction* action) {
    return syscall2(SYS_SIGACTION, (long long) sig, (long long) action);
}

long process_set_signal_disposition(signal sig,
                                    signal_disposition disposition) {

    return syscall2(SYS_SET_SIGNAL_DISPOSITION, (long long) sig,
                    (long long) disposition);
}