#include "signal.h"
#include "syscall.h"

int signals = 0;

void exit(long long number) { syscall1(SYS_EXIT, number); }

void print(const char* str) { syscall1(SYS_PRINT, (long long) str); }

void printll(long long num) { syscall1(SYS_PRINT_U64, num); }

void trigger_segfault() { *(volatile long*) 0 = 1; }

void sigint_handler() {
    print("Hello from signal handler!\n");
    signals++;

    if (signals == 12)
        trigger_segfault();
}

const sigaction sigint_action = {.handler = (signal_handler*) sigint_handler};
const sigaction sigkill_action = {.disposition = IGNORE};

void __attribute__((section(".entrypoint"))) main() {
    // This one should succeed (e.g. return 0)
    long sigint_act_set = set_sigaction(SIGINT, &sigint_action);
    // This one should fail (e.g. return value < 0)
    long sigkill_act_set = set_sigaction(SIGKILL, &sigkill_action);

    for (volatile long long i = 0;; i++) {
        if (i % 100000 == 0) {
            print("user space, sigint: ");
            printll(sigint_act_set);
            print(", sigkill: ");
            printll(sigkill_act_set);
            print(", cnt: ");
            printll(signals);
            print("\n");
        }

        if (signals == 50)
            break;
    }

    print("EXITING!\n");
    exit(0xDEADB33F);
}