#ifndef SOS_PROCESS_H
#define SOS_PROCESS_H

#include "../lib/container/array_list/array_list.h"
#include "../memory/memory_map.h"
#include "../memory/virtual/vm.h"
#include "../signal/signal.h"
#include "../synchronization/spin_lock.h"

struct thread;

typedef struct {
    sigpending pending_signals;
    signal_disposition dispositions[SIGNALS_COUNT + 1];
} process_siginfo;

typedef struct {
    u64 id;
    bool kernel_process;
    vm_space* vm;

    // Node to use in process cleaner to avoid memory allocations that may
    // potentially crash
    linked_list_node cleaner_node;

    lock lock; // guards all fields below
    bool exiting;
    bool finished;
    u64 exit_code;

    process_siginfo siginfo;
    array_list threads;

    ref_count refc;
    con_var finish_cvar;
} process;

bool process_init(process* proc, bool kernel_process);
void process_destroy(process* proc);

bool process_signal(process* proc, signal sig);
bool process_add_thread(process* proc, struct thread* thrd);
void process_kill(process* proc);

// These functions should be called from process
bool process_exit_thread(); // returns whether this is the last thread and it
                            // should clean process
void process_exit(u64 exit_code);

signal_disposition process_get_signal_disposition(signal sig);
bool process_set_signal_disposition(signal sig, signal_disposition disposition);

#endif // SOS_PROCESS_H
