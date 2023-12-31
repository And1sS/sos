#include "../../../lib/memory_util.h"
#include "../../common/signal.h"
#include "../cpu/cpu_context.h"

// TODO: Add checks for rsp that it is present and mapped
#define STACK_PUSH(rsp, type, val)                                             \
    do {                                                                       \
        rsp -= sizeof(type);                                                   \
        *(type*) rsp = val;                                                    \
    } while (0)

/*
 * Trampoline for signal handler, which lives on the signal handling thread
 * stack in user space. This bytecode is encoded x86-64 assembly: mov rax, 4;
 * int 0x80
 */
const u8 SIGRET_TRAMPOLINE_CODE[] = {0x48, 0xC7, 0xC0, 0x04, 0x00,
                                     0x00, 0x00, 0xCD, 0x80};
const u8 SIGRET_TRAMPOLINE_SIZE = 9;

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
    arch_context->rsp -= sizeof(cpu_context);
    memcpy((void*) arch_context->rsp, context, sizeof(cpu_context));

    arch_context->rsp -= SIGRET_TRAMPOLINE_SIZE;
    memcpy((void*) arch_context->rsp, (void*) SIGRET_TRAMPOLINE_CODE,
           SIGRET_TRAMPOLINE_SIZE);

    u64 rsp = arch_context->rsp;
    STACK_PUSH(arch_context->rsp, u64, rsp);

    arch_context->rip = (u64) handler;
}

void arch_return_from_signal_handler(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;
    u64 rsp = arch_context->rsp + SIGRET_TRAMPOLINE_SIZE;

    memcpy((void*) context, (void*) rsp, sizeof(cpu_context));
    arch_context->rsp += sizeof(cpu_context);
}
