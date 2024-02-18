#include "../../../lib/memory_util.h"
#include "../../../threading/kthread.h"
#include "../../../threading/uthread.h"
#include "../cpu/cpu_context.h"
#include "../cpu/gdt.h"
#include "../cpu/rflags.h"

void kernel_thread_wrapper(kthread_func* func) {
    func();
    thread_exit(0);
}

struct cpu_context* arch_kthread_context_init(kthread* thrd,
                                              kthread_func* func) {

    u64 start_rsp = (u64) thrd->kernel_stack + THREAD_KERNEL_STACK_SIZE;

    cpu_context* context = (cpu_context*) (start_rsp - sizeof(cpu_context));
    memset(context, 0, sizeof(cpu_context));

    context->rip = (u64) kernel_thread_wrapper;
    context->cs = KERNEL_CODE_SEGMENT_SELECTOR;
    context->rflags = RFLAGS_IRQ_ENABLED_FLAG | RFLAGS_INIT_FLAGS;
    context->rsp = start_rsp;
    context->ds = KERNEL_DATA_SEGMENT_SELECTOR;
    context->ss = KERNEL_DATA_SEGMENT_SELECTOR;
    context->rdi = (u64) func;

    return (struct cpu_context*) context;
}

struct cpu_context* arch_uthread_context_init(uthread* thrd,
                                              uthread_func* func) {

    u64 start_rsp = (u64) thrd->user_stack + THREAD_KERNEL_STACK_SIZE;

    u64 kernel_rsp = (u64) thrd->kernel_stack + THREAD_KERNEL_STACK_SIZE;
    // On x86-64 cpu context always lives on kernel stack
    cpu_context* context = (cpu_context*) (kernel_rsp - sizeof(cpu_context));
    memset(context, 0, sizeof(cpu_context));

    context->rip = (u64) func;
    context->cs = USER_CODE_SEGMENT_SELECTOR;
    context->rflags = RFLAGS_IRQ_ENABLED_FLAG | RFLAGS_INIT_FLAGS;
    context->rsp = start_rsp;
    context->ds = USER_DATA_SEGMENT_SELECTOR;
    context->ss = USER_DATA_SEGMENT_SELECTOR;

    return (struct cpu_context*) context;
}