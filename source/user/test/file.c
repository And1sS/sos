
#include "syscall.h"

long long int open(const char* path, unsigned long long flags) {
    return syscall2(SYS_OPEN, (long long) path, flags);
}

long long int close(unsigned long long fd) {
    return syscall1(SYS_CLOSE, (long long) fd);
}

long long int read(unsigned long long fd, void* buffer,
                   unsigned long long size) {

    return syscall3(SYS_READ, (long long) fd, (long long) buffer,
                    (long long) size);
}

long long int write(unsigned long long fd, void* buffer,
                    unsigned long long size) {

    return syscall3(SYS_WRITE, (long long) fd, (long long) buffer,
                    (long long) size);
}