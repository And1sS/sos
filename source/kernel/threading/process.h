#ifndef SOS_PROCESS_H
#define SOS_PROCESS_H

#include "../lib/container/array_list/array_list.h"
#include "../memory/memory_map.h"
#include "../memory/virtual/vm.h"
#include "../synchronization/spin_lock.h"

struct thread;

typedef struct {
    u64 id;
    bool kernel_process;
    vm_space* vm;

    // Node to use in process cleaner to avoid memory allocations that may
    // potentially crash
    linked_list_node cleaner_node;

    lock lock;
    bool finishing;
    u64 exit_code;

    array_list thread_groups;

    ref_count refc;
} process;

bool process_init(process* proc, bool kernel_process);
void process_destroy(process* proc);

bool process_add_thread_group(process* proc, struct thread* thrd);

// returns whether this is the last thread and it should clean process
bool process_exit_thread(process* proc, struct thread* thrd);

#endif // SOS_PROCESS_H
