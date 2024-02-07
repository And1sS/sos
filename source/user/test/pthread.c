#include "pthread.h"
#include "syscall.h"

pthread* pthread_run(const char* name, pthread_func* func) {
    return (pthread*) syscall2(SYS_PTHREAD_RUN, (long long) name,
                               (long long) func);
}

long long pthread_join(pthread* thread) {
    return syscall1(SYS_PTHREAD_JOIN, (long long) thread);
}

long long pthread_detach(pthread* thread) {
    return syscall1(SYS_PTHREAD_DETACH, (long long) thread);
}