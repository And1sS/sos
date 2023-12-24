#include "../../../lib/types.h"
#include "../../../signal/arch_signal.h"
#include "../cpu/cpu_context.h"
#include "../cpu/gdt.h"

bool arch_is_userspace_context(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;
    return arch_context->cs == USER_CODE_SEGMENT_SELECTOR;
}

/*
 * This function is called when user is coming into user space, so context
 * contains user rip and rsp like so:
 * kernel stack (where saved context resides):
 *      stack growth direction <--- | ... | rip | .. | rsp |
 * user stack(which address is stored in context->rsp):
 *      stack growth direction <--- | ... |
 *
 * To install signal handler we need to place current context->rip as return
 * address on stack (emulating function call) and set context->rip to signal
 * handler address. After that user stack will look like this: user stack:
 *      stack growth direction <--- | rip | ... |
 */
void arch_install_user_signal_handler(struct cpu_context* context,
                                      u64 signal_handler) {

    cpu_context* arch_context = (cpu_context*) context;
    arch_context->rsp -= 8;
    *(u64*) arch_context->rsp = arch_context->rip;

    arch_context->rip = signal_handler;
}
