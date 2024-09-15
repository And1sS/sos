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

void sigchld_handler() {
    print("Process: ");
    printll(getpid());
    print(" sigchld handler: ");

    long long exit_code;
    long long exit_pid = wait(0, &exit_code);

    if (exit_pid < 0) {
        print("no child processes to free\n");
        return;
    }

    print(" freed child: ");
    printll(exit_pid);
    print(", code: ");
    printll(exit_code);
    print("\n");
}

int threads = 1;

void thread_func() {
    long long i = 1;
    int current = ++threads;
    long long pid = getpid();

    while (1) {
        print("Hello from proc: ");
        printll(pid);
        print(", thread: ");
        printll(current);
        print(", cnt: ");
        printll(i);
        print("\n");

        if (i == 5)
            break;

        i++;
    }

    pthread_exit(0xCAFE);
}

const sigaction sigint_action = {.handler = (signal_handler*) sigint_handler};
const sigaction sigkill_action = {.disposition = IGNORE};
const sigaction sigchld_action = {.handler = (signal_handler*) sigchld_handler};

void __attribute__((section(".entrypoint"))) main() {
    // This one should succeed (e.g. return 0)
    long sigint_act_set = process_set_sigaction(SIGINT, &sigint_action);
    // This one should fail (e.g. return value < 0)
    long sigkill_act_set = process_set_sigaction(SIGKILL, &sigkill_action);
//    long sigchld_act_set = process_set_sigaction(SIGCHLD, &sigchld_action);

    for (int i = 0; i < 11; i++) {
        if (fork() < 0)
            exit(-2);
    }

    print("Process: ");
    printll(getpid());
    print(" started\n");

#define THREADS_COUNT 3
    pthread threads[THREADS_COUNT];
    int cnt = 0;
    for (; cnt < THREADS_COUNT; cnt++) {
       if (pthread_run("other-thread", thread_func, &threads[cnt]) != 0)
           break;
    }

    for (long long i = 0; i < cnt; i++) {
        long long exit_code;
        pthread_join(threads[i], &exit_code);
    }

    long long exit_code;
    long long exit_pid;

    while ((exit_pid = wait(0, &exit_code)) > 0) {
        print("Process: ");
        printll(getpid());
        print(", child exited: ");
        printll(exit_pid);
        print(", with code: ");
        printll(exit_code);
        print("\n");
    }

    // Parent waits forever
    if (getpid() == 1) {
        for (;;)
            ;
    }

    print("Process: ");
    printll(getpid());
    print(" exiting\n");
    exit(0xDEADB33F);
}