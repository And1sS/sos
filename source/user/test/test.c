#include "syscall.h"

int signals = 0;

void install_signal_handler(void* handler) {
    syscall1(SYS_SIGINSTALL, (long long) handler);
}

void exit(long long number) { syscall1(SYS_EXIT, number); }

void print(const char* str) { syscall1(SYS_PRINT, (long long) str); }

void printll(long long num) { syscall1(SYS_PRINT_U64, num); }

void trigger_segfault() { *(volatile long*) 0 = 1; }

void signal_handler() {
    print("Hello from signal handler!\n");
    signals++;

    if (signals == 12)
        trigger_segfault();
}

void __attribute__((section(".entrypoint"))) main() {
    install_signal_handler(signal_handler);

    for (volatile long long i = 0;; i++) {
        if (i % 100000 == 0) {
            print("user space: ");
            printll(signals);
            print("\n");
        }

        if (signals == 50)
            break;
    }

    print("EXITING!\n");
    exit(0xDEADB33F);
}