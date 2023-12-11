#ifndef SOS_THREAD_H
#define SOS_THREAD_H

#include "../lib/container/linked_list/linked_list.h"
#include "../lib/types.h"
#include "../synchronization/spin_lock.h"

#define THREAD_STACK_SIZE 8192

typedef void thread_func();

typedef enum {
    INITIALISED = 0,
    RUNNING = 1,
    STOPPED = 2,
    BLOCKED = 3,
    DEAD = 4
} thread_state;

struct cpu_context;

typedef struct {
    u64 id;
    string name;

    struct cpu_context* context;
    void* stack;
    thread_state state;

    lock lock; // for now only guards against de-allocation, but later will be
               // used to guard this struct
    linked_list_node scheduler_node; // this is used in scheduler and thread
                                     // cleaner, never changes
} thread;

void threading_init();

thread* thread_create(string name, thread_func* func);
thread* thread_run(string name, thread_func* func);

void thread_start(thread* thread);

// for internal usage
void thread_exit();
void thread_destroy(thread* thread);

#endif // SOS_THREAD_H
