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

typedef struct {
    signal_handler* handler;
} sigaction;

typedef struct {
    sigpending pending_signals;
    sigmask signals_mask;
    sigaction signal_actions[SIGNALS_COUNT + 1];
} thread_siginfo;

struct cpu_context;

typedef struct _thread {
    u64 id;
    string name;
    bool kernel_thread;
    void* kernel_stack;
    void* user_stack;
    process* proc;

    // context should be accessed only within the scheduler, so no locking
    // required
    struct cpu_context* context;

    linked_list_node scheduler_node; // this is used in scheduler and thread
                                     // cleaner, never changes

    lock lock; // guards fields below and also guards thread against
               // de-allocation
               // lock order:
               // 1) process -> thread
               // 2) parent -> child

    // state should be modified only within the thread
    thread_state state;

    bool on_scheduler_queue;
    bool exiting;
    bool finished;
    u64 exit_code;

    thread_siginfo siginfo;

    u64 should_die; // Used in kernel threads - just a testable flag.

    ref_count refc;

    struct _thread* parent;
    array_list children; // children threads

    con_var finish_cvar;
} thread;

void thread_start(thread* thread);

// These functions should be called within thread
bool thread_add_child(thread* child);
void thread_remove_child(thread* child);
bool thread_detach(thread* child);
bool thread_join(thread* child, u64* exit_code);

bool thread_set_sigaction(signal sig, sigaction action);
sigaction thread_get_sigaction(signal sig);

bool thread_signal_block(signal sig);
void thread_signal_unblock(signal sig);

void thread_exit(u64 exit_code);
void thread_yield();

// This function should be called from thread cleaner
void thread_destroy(thread* thread);

bool thread_signal(thread* thread, signal sig);
bool thread_signal_if_allowed(thread* thrd, signal sig);
bool thread_signal_allowed(thread* thrd, signal sig);

/*
 * Acquires and releases thread lock. Should be used when releasing
 * references guarded by thread lock.
 */
void thread_lock_guarded_refc_release(thread* thrd, ref_count* refc);

#endif // SOS_THREAD_H
