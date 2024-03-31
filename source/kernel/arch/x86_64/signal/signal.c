#include "../../common/signal.h"
#include "../../../memory/virtual/umem.h"
#include "../../../threading/thread.h"
#include "../cpu/cpu_context.h"
#include "../cpu/gdt.h"
#include "../cpu/rflags.h"

#define STACK_PUSH(rsp, type, val)                                             \
    do {                                                                       \
        type __val = val;                                                      \
        rsp -= sizeof(type);                                                   \
        if (!copy_to_user((void*) (rsp), &__val, sizeof(type)))                \
            thread_exit(-1);                                                   \
    } while (0)

#define STACK_PUSH_RAW(rsp, size, src)                                         \
    do {                                                                       \
        rsp -= size;                                                           \
        if (!copy_to_user((void*) (rsp), (void*) (src), size))                 \
            thread_exit(-1);                                                   \
    } while (0)

extern void signal_trampoline_code_start();
extern void signal_trampoline_code_end();
#define TRAMPOLINE_CODE_SIZE                                                   \
    ((u64) signal_trampoline_code_end - (u64) signal_trampoline_code_start)

/*
 * This function installs signal handler and is called when user is coming into
 * user space, so context contains user rip and rsp like so:
 * kernel stack (where saved context resides):
 *      stack growth direction <--- | ... | rip | .. | rsp |
 * user stack(which address is stored in context->rsp):
 *      stack growth direction <--- | ... |
 *
 * To install signal handler we need to:
 *      1. save current state on stack
 *      2. place signal trampoline code
 *      3. then place signal trampoline code address, so that returning from
 *      signal handler will cause cpu to start executing signal trampoline code,
 *      which will make syscall for signal return, which in turn will restore
 * saved state
 *      4. set context->rip to signal handler address.
 *
 * After that user stack will look like this:
 * user stack:
 *      stack growth direction <--- | tramp addr | tramp code | state | ...
 */
void arch_enter_signal_handler(struct cpu_context* context,
                               signal_handler* handler) {

    cpu_context* arch_context = (cpu_context*) context;

    STACK_PUSH(arch_context->rsp, cpu_context, *arch_context);
    STACK_PUSH_RAW(arch_context->rsp, TRAMPOLINE_CODE_SIZE,
                   signal_trampoline_code_start);
    STACK_PUSH(arch_context->rsp, u64, arch_context->rsp);

    arch_context->rip = (u64) handler;
}

/*
 * This function is counterpart of arch_enter_signal_handler.
 *
 * To enter this function, user space jumped to signal trampoline code by
 * popping trampoline address from the stack, so for now user stack looks like
 * this:
 * user stack(which address is stored in context->rsp):
 *      stack growth direction <--- tramp code | state | ...
 *
 * To restore previous state we just need increase rsp on size of trampoline
 * code and then restore saved cpu context from user stack.
 *
 * Note: cs, ss and flags registers should be copied with care, since we should
 * not let user enter kernel space or disable interrupts.
 * Since this function is needed to implement syscall for returning from signal
 * handler, rax register will be scratched by syscall return value. To preserve
 * it, rax is returned.
 */
u64 arch_return_from_signal_handler(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;

    void* src = (void*) arch_context->rsp + TRAMPOLINE_CODE_SIZE;
    if (!copy_from_user((void*) context, src, sizeof(cpu_context)))
        thread_exit(-1);

    // make sure user space did not modify cs, ss and flags to mess kernel state
    arch_context->ss = USER_DATA_SEGMENT_SELECTOR;
    arch_context->cs = USER_CODE_SEGMENT_SELECTOR;
    arch_context->rflags |= RFLAGS_IRQ_ENABLED_FLAG | RFLAGS_INIT_FLAGS;

    return arch_context->rax;
}
