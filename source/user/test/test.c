#include "syscall.h"

void exit(long long number) { syscall1(SYS_EXIT, number); }

void print(const char* str) { syscall1(SYS_PRINT, (long long) str); }

void __attribute__((section(".entrypoint"))) main() {
    for (int i = 0; i < 1000; i++) {
        print("Test!!!!");
    }

    exit(0xDEADB33F);
}