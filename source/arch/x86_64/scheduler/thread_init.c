#include "../../../lib/memory_util.h"
#include "../../../scheduler/thread.h"
#include "../cpu/cpu_context.h"
#include "../cpu/gdt.h"
#include "../cpu/rflags.h"

void kernel_thread_wrapper(thread_func* func) {
    func();
    thread_exit();
}

struct cpu_context* arch_thread_context_init(thread* thrd, thread_func* func) {
    u64 start_rsp = (u64) thrd->stack + THREAD_STACK_SIZE;

    cpu_context* context = (cpu_context*) (start_rsp - sizeof(cpu_context));
    memset(context, 0, sizeof(cpu_context));

    context->rip = (u64) kernel_thread_wrapper;
    context->cs = KERNEL_CODE_SEGMENT_SELECTOR;
    context->rflags = RFLAGS_IRQ_ENABLED_FLAG | RFLAGS_INIT_FLAGS;
    context->rsp = start_rsp;
    context->ss = KERNEL_DATA_SEGMENT_SELECTOR;
    context->rdi = (u64) func;

    return (struct cpu_context*) context;
}