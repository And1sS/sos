#ifndef SOS_THREAD_H
#define SOS_THREAD_H

#include "../lib/container/array_list/array_list.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../signal/signal.h"
#include "../synchronization/completion.h"
#include "../synchronization/spin_lock.h"

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

    // state and context should be modified only within the thread, so no
    // locking required on accesses, visibility is carried by scheduler
    thread_state state;
    struct cpu_context* context;

    linked_list_node scheduler_node; // this is used in scheduler and thread
                                     // cleaner, never changes

    lock lock; // guards fields below and also guards thread against
               // de-allocation

    bool exiting;
    bool finished;
    u64 exit_code;

    u64 pending_signals; // bitmap of pending signals
    u64 signals_mask;    // bitmap of blocked signals, 0 - signal blocked,
                         // 1 - signal unblocked
    signal_handler* signal_handler;
    struct cpu_context* signal_enter_context; // copy of context before
                                              // executing signal handler

    ref_count refc;

    bool should_die; // used in kernel threads

    // here should be pointer to process

    struct _thread* parent;
    array_list children; // children threads

    con_var finish_cvar;
} thread;

void thread_start(thread* thread);

void thread_detach(thread* thread);
u64 thread_join(thread* thread);

void thread_exit(u64 exit_code);
void thread_destroy(thread* thread);

void thread_yield();

bool thread_signal(thread* thread, signal sig);

#endif // SOS_THREAD_H
