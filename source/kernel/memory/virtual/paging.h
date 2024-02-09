#ifndef SOS_PAGING_H
#define SOS_PAGING_H

#include "../../lib/kprint.h"
#include "../../scheduler/scheduler.h"
#include "../../threading/thread.h"

struct cpu_context* handle_page_fault(struct cpu_context* context) {
    thread* current = get_current_thread();
    if (!current || current->kernel_thread) {
        panic("Unhandled page fault");
    } else {
        thread_signal(current, SIGSEGV);
    }

    return context;
}

#endif // SOS_PAGING_H
