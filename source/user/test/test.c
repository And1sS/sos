#include "exit.h"
#include "fork.h"
#include "getpid.h"
#include "pthread.h"
#include "signal.h"
#include "syscall.h"
#include "wait.h"

int signals = 0;

void print(const char* str) { syscall1(SYS_PRINT, (long long) str); }

void printll(long long num) { syscall1(SYS_PRINT_U64, num); }

void trigger_segfault() { *(volatile long*) 0 = 1; }

void sigint_handler() {
    print("Hello from signal handler!\n");
    signals++;
}

int threads = 1;

void thread_func() {
    long long i = 1;
    int current = ++threads;

    while (1) {
        print("Hello from thread ");
        printll(current);
        print(", cnt: ");
        printll(i);
        print("\n");

        if (i == 500)
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

    //    pthread thread;
    //    for (long long i = 0; i < 10; i++) {
    //        pthread_run("other-thread", thread_func, &thread);
    //    }

    long long pid = fork();
    long long pid1 = fork();
    long long pid2 = fork();

    if (pid < 0) {
        print("ERROR");
        exit(-1);
    }

    long long exit_code;
    long long exit_pid;

    while ((exit_pid = wait(0, &exit_code)) > 0) {
        print("Proc: ");
        printll(getpid());
        print(", child exited: ");
        printll(exit_pid);
        print(", with code: ");
        printll(exit_code);
        print("\n");
    }

    if (pid != 0 && pid1 != 0 && pid2 != 0) {
        for (;;);
    }
    //    const char* to_print = pid == 0 ? "CHILD! " : "PARENT! ";
    //
    //    for (;;) {
    //        print(to_print);
    //        printll(pid);
    //        print("\n");
    //    }

    print("EXITING!\n");
    exit(0xDEADB33F);
}