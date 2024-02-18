#include "pthread.h"
#include "syscall.h"

long long pthread_run(const char* name, pthread_func* func, pthread* thread) {
    return syscall3(SYS_PTHREAD_RUN, (long long) name, (long long) func,
                    (long long) thread);
}

long long pthread_join(pthread thread, long long* exit_code) {
    return syscall2(SYS_PTHREAD_JOIN, (long long) thread,
                    (long long) exit_code);
}

long long pthread_detach(pthread thread) {
    return syscall1(SYS_PTHREAD_DETACH, (long long) thread);
}
