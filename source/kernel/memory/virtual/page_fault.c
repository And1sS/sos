#include "page_fault.h"
#include "../../arch/common/context.h"
#include "../../error/errno.h"
#include "../../scheduler/scheduler.h"

struct cpu_context* handle_page_fault(struct cpu_context* context) {
    thread* current = get_current_thread();
    if (!current || current->kernel_thread
        || !arch_is_userspace_context(context)) {

        arch_print_cpu_context(context);
        panic("Unhandled page fault");
    } else {
        thread_signal(current, SIGSEGV);
    }

    return context;
}
