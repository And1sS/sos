#ifndef SOS_THREAD_H
#define SOS_THREAD_H

#include "../lib/container/array_list/array_list.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../signal/signal.h"
#include "../synchronization/completion.h"
#include "../synchronization/spin_lock.h"
#include "process.h"

#define THREAD_KERNEL_STACK_SIZE 8192

typedef enum {
    INITIALISED = 0,
    RUNNING = 1,
    STOPPED = 2,
    BLOCKED = 3,
    DEAD = 4
} thread_state;

struct cpu_context;

typedef struct _thread {
    u64 id;
    string name;
    bool kernel_thread;
    void* kernel_stack;
    void* user_stack;
    process* proc;

    // context should be accessed only within the thread, so no locking required
    struct cpu_context* context;

    linked_list_node scheduler_node; // this is used in scheduler and thread
                                     // cleaner, never changes

    lock lock; // guards fields below and also guards thread against
               // de-allocation

    // state should be modified only within the thread
    thread_state state;

    bool on_scheduler_queue;
    bool exiting;
    bool finished;
    u64 exit_code;

    siginfo signal_info;

    ref_count refc;

    bool should_die; // used in kernel threads

    struct _thread* parent;
    array_list children; // children threads

    con_var finish_cvar;
} thread;

void thread_start(thread* thread);

// These functions should be called within thread
bool thread_add_child(thread* child);
bool thread_detach(thread* child);
bool thread_join(thread* child, u64* exit_code);

void thread_exit(u64 exit_code);
void thread_yield();

// This function should be called from thread cleaner
void thread_destroy(thread* thread);

bool thread_signal(thread* thread, signal sig);
bool thread_set_sigaction(thread* thread, signal sig, sigaction action);

#endif // SOS_THREAD_H
