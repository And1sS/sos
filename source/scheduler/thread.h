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

typedef struct {
    u64 id;
    string name;

    u64 stack_pointer;
    void* stack;
    thread_state state;
} thread;

void init_thread_creator();
thread* init_thread(thread* thrd, string name, thread_func* func);

#endif // SOS_THREAD_H
