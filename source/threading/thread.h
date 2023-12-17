#ifndef SOS_THREAD_H
#define SOS_THREAD_H

#include "../lib/container/array_list/array_list.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
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

    // here should be pointer to process

    bool exiting;
    bool finished;
    u64 exit_code;

    ref_count refc;

    bool kernel_thread;
    bool should_die; // used in kernel threads

    struct cpu_context* context;
    void* stack;
    thread_state state;

    struct _thread* parent;
    array_list children; // children threads

    con_var finish_cvar;

    lock lock; // guards above fields and also guards thread against
               // de-allocation

    linked_list_node scheduler_node; // this is used in threading and thread
                                     // cleaner, never changes
} thread;

void thread_start(thread* thread);

void thread_detach(thread* thread);
u64 thread_join(thread* thread);

void thread_exit(u64 exit_code);
void thread_destroy(thread* thread);

void thread_yield();

#endif // SOS_THREAD_H
