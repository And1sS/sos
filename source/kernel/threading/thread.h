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

    // these fields are used in scheduler
    thread_state state; // should be modified within thread
                        // TODO: guard writes to this field with barriers to
                        //       prevent reordering, probably make it u64 and
                        //       change accesses to atomic_read/atomic_write
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

    siginfo signal_info;

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
bool thread_any_pending_signals();

bool thread_signal(thread* thread, signal sig);
bool thread_set_sigaction(thread* thread, signal sig, sigaction action);

// Exists mainly to simplify thread unlocking on exiting.
void thread_unlock(thread* thread);

#endif // SOS_THREAD_H
