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

struct cpu_context;

typedef enum {
    INITIALISED = 0,
    RUNNING = 1,
    STOPPED = 2,
    BLOCKED = 3,
    DEAD = 4
} thread_state;

typedef struct {
    sigpending pending_signals;
    sigmask signals_mask;
} thread_siginfo;

/*
 * Thread locking order:
 * 1) thread lock
 * 2) thread signals lock
 */
typedef struct _thread {
    // Immutable data
    u64 id;   // global thread id
    u64 tgid; // id inside thread group (process)
    string name;
    bool kernel_thread;
    // End of immutable data

    void* kernel_stack;
    void* user_stack;

    process* proc;

    thread_siginfo siginfo;
    lock siginfo_lock; // guards siginfo

    // these fields are used in scheduler
    thread_state state; // should be modified within thread
    // these fields should be modified only by scheduler
    struct cpu_context* context;
    bool currently_running;
    bool on_run_queue;
    linked_list_node scheduler_node;

    lock lock; // guards fields below and also guards thread against
               // de-allocation

    bool exiting;
    bool finished;
    u64 exit_code;

    ref_count refc;

    bool should_die; // used in kernel threads

    struct _thread* parent;
    array_list children; // children threads

    con_var finish_cvar;
} thread;

void threading_init();

bool threading_allocate_tid(u64* result);
bool threading_free_tid(u64 tid);

void thread_start(thread* thread);

bool thread_add_child(thread* child);
void thread_remove_child(thread* child);

bool thread_detach(thread* thread);
bool thread_join(thread* thread, u64* exit_code);

_Noreturn void thread_exit(u64 exit_code);
void thread_destroy(thread* thread);

void thread_yield();

bool thread_signal(thread* thread, signal sig);
bool thread_any_pending_signals();

// Exists mainly to simplify thread unlocking on exiting.
void thread_unlock(thread* thread);

#endif // SOS_THREAD_H
