#ifndef SOS_PROCESS_H
#define SOS_PROCESS_H

#include "../lib/container/array_list/array_list.h"
#include "../lib/id_generator.h"
#include "../memory/memory_map.h"
#include "../memory/virtual/vm.h"
#include "../signal/signal.h"
#include "../synchronization/spin_lock.h"

struct thread;

typedef struct {
    sigpending pending_signals;
    sigaction actions[SIGNALS_COUNT + 1];
} process_siginfo;

/*
 * Process locking order:
 * 1) process lock
 * 2) process signals lock
 */
typedef struct _process {
    // Immutable data
    u64 id;
    bool kernel_process;
    // End of immutable data

    vm_space* vm;

    process_siginfo siginfo;
    lock siginfo_lock; // guards siginfo

    lock lock; // guards all fields below
    bool exiting;
    bool finished;
    u64 exit_code;

    id_generator tgid_generator;
    array_list threads;

    struct _process* parent;

    linked_list_node process_node; // node that will be used in parent process
                                   // and in process cleaner to store current
                                   // process  in fail-safe manner

    linked_list children; // stores process_node of children

    ref_count refc;
    con_var finish_cvar;
} process;

void processing_init();

u64 process_fork(struct cpu_context* context);
u64 process_wait(u64 pid, u64* exit_code);

bool process_signal(process* proc, signal sig);
bool process_add_thread(process* proc, struct thread* thrd);
void process_kill(process* proc);

// These functions should be called from process
_Noreturn void process_exit_thread();
_Noreturn void process_exit(u64 exit_code);

bool process_set_sigaction(signal sig, sigaction action);
sigaction process_get_sigaction(signal sig);
bool process_any_pending_signals();

#endif // SOS_PROCESS_H
