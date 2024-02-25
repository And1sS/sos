#include "exit.h"
#include "pthread.h"
#include "signal.h"
#include "syscall.h"

int signals = 0;

void print(const char* str) { syscall1(SYS_PRINT, (long long) str); }

void printll(long long num) { syscall1(SYS_PRINT_U64, num); }

void trigger_segfault() { *(volatile long*) 0 = 1; }

void sigint_handler() {
    print("Hello from signal handler!\n");
    signals++;
}

void second_thread_func() {
    long long i = 1;
    while (1) {

        print("Hello from second thread!  ");
        printll(i);
        print("\n");

        if (i == 3000)
            break;

        i++;
    }

    pthread_exit(0xCAFE);
}

const sigaction sigint_action = {.handler = (signal_handler*) sigint_handler};
const sigaction sigkill_action = {};

void __attribute__((section(".entrypoint"))) main() {
    process_set_signal_disposition(SIGINT, IGNORE);

    // This one should succeed (e.g. return 0)
    long sigint_act_set = pthread_sigaction(SIGINT, &sigint_action);
    // This one should fail (e.g. return value < 0)
    long sigkill_act_set = pthread_sigaction(SIGKILL, &sigkill_action);

    pthread thread;
    pthread_run("second-test-thread", second_thread_func, &thread);

    long long printed = 0;
    for (volatile long long i = 0;; i++) {
        if (i % 100000 == 0) {
            print("user space, SIGINT cnt: ");
            printll(signals);
            print(", print cnt: ");
            printll(printed);
            print("\n");
            printed++;
        }

        if (signals == 50)
            break;
    }

    print("EXITING!\n");
    exit(0xDEADB33F);
}