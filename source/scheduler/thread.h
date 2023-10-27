#ifndef SOS_THREAD_H
#define SOS_THREAD_H

#include "../lib/types.h"

#define THREAD_STACK_SIZE 8192

typedef void thread_func();

typedef enum {
    INITIALISED = 0,
    RUNNING = 1,
    STOPPED = 2,
    BLOCKED = 3
} thread_state;

typedef struct __attribute__((__packed__)) {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;

    void* kernel_wrapper_func;
    void* unused_return_addr;
    thread_func* init_func;
} thread_context;

typedef struct {
    u64 rsp;

    u64 id;
    string name;

    void* stack;
    thread_state state;
} thread;

#endif // SOS_THREAD_H
