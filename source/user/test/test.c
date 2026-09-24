#include "exit.h"
#include "file.h"
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

void run_threads() {
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
}

void fork_test() {
    run_threads();

    for (int i = 0; i < 11; i++) {
        if (fork() < 0)
            exit(-2);
    }

    run_threads();

    print("Process: ");
    printll(getpid());
    print(" started\n");

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

void io_test() {
    print("=========== IO test ============\n");
    const char* path = "a/c/d/f";

    long long fd = open(path, 0);
    print("Opened file at: ");
    print(path);
    print(", fd: ");
    printll(fd);
    print("\n");

    const char test[] = "Hello from write syscall!!";
    long long written_bytes = write(fd, test, 26);
    if (written_bytes >= 0) {
        print("Successfully written ");
        printll(written_bytes);
        print(" bytes of buffer: ");
        print(test);
    } else {
        print("Couldn't write buffer to file with err code: ");
        printll(written_bytes);
    }
    print("\n");
    close(fd);

    const char buf[256];
    fd = open("a/c/d/f", 0);
    long long read_bytes = read(fd, buf, 256);
    print("Successfully read ");
    printll(read_bytes);
    print(" bytes, buffer: ");
    print(buf);
    print("\n");

    const long long fd1 = open(".", 0);
    print("Next fd: ");
    printll(fd1);
    print("\n");
    close(fd);
    close(fd1);

    long long res = chdir("/a/c");
    if (res < 0) {
        print("Couldn't change workdir to /a/c");
        exit(-1);
    }

    print("Successfully changed workdir to /a/c\n");
    fd = open("d/f", 0);
    if (fd < 0) {
        print("Failed to open d/f");
        exit(-1);
    }
    print("Successfully opened d/f\n");
    read_bytes = read(fd, buf, 256);
    print("Successfully read ");
    printll(read_bytes);
    print(" bytes, buffer: ");
    print(buf);
    print("\n");
    close(fd);

    res = chroot("a/c/d");
    if (res < 0) {
        print("Failed to change root to a/c/d");
        exit(-1);
    }

    print("Successfully changed root to /a/c/d\n");
    fd = open("/f", 0);
    if (fd < 0) {
        print("Failed to open /f");
        exit(-1);
    }
    print("Successfully opened /f\n");
    read_bytes = read(fd, buf, 256);
    print("Successfully read ");
    printll(read_bytes);
    print(" bytes, buffer: ");
    print(buf);
    print("\n");
    close(fd);

    for (;;)
        ;
}

void __attribute__((section(".entrypoint"))) main() {
    // This one should succeed (e.g. return 0)
    long sigint_act_set = process_set_sigaction(SIGINT, &sigint_action);
    // This one should fail (e.g. return value < 0)
    long sigkill_act_set = process_set_sigaction(SIGKILL, &sigkill_action);
    long sigchld_act_set = process_set_sigaction(SIGCHLD, &sigchld_action);

    io_test();
    //    fork_test();
    while (1)
        ;
}