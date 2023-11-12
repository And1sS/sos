#include "../../../lib/memory_util.h"
#include "../../../scheduler/scheduler.h"
#include "../gdt.h"

typedef struct __attribute__((__packed__)) {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} cpu_context;

void kernel_thread_wrapper(thread_func* func) {
    func();
    schedule_thread_exit();
}

struct cpu_context* arch_thread_context_init(thread* thrd, thread_func* func) {
    u64 start_rsp = (u64) thrd->stack + THREAD_STACK_SIZE - sizeof(cpu_context);

    cpu_context* context = (cpu_context*) start_rsp;
    memset(context, 0, sizeof(cpu_context));

    context->rip = (u64) kernel_thread_wrapper;
    context->cs = KERNEL_CODE_SEGMENT_SELECTOR;
    context->rflags = 0x202; // interrupts enabled + bit 2 reserved and should
                             // be set to 1
                             // TODO: add constants
    context->rsp = start_rsp;
    context->ss = KERNEL_DATA_SEGMENT_SELECTOR;
    context->rdi = (u64) func;

    return (struct cpu_context*) start_rsp;
}