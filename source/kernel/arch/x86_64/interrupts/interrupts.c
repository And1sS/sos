#include "interrupts.h"
#include "../../../memory/virtual/page_fault.h"
#include "../../../syscall/syscall.h"
#include "../../../threading/scheduler.h"
#include "../../../time/timer.h"
#include "../cpu/cpu_context.h"
#include "../timer/pit.h"
#include "idt.h"
#include "isrs.h"
#include "pic.h"

struct cpu_context* syscall_trampoline(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;

    arch_context->rax =
        handle_syscall(arch_context->rdi, arch_context->rsi, arch_context->rdx,
                       arch_context->rcx, arch_context->r8, arch_context->r9,
                       arch_context->rax, context);

    return context;
}

void interrupts_init() {
    idt_init();
    pit_init();
    pic_init();

    mount_exception_handler(14, handle_page_fault);

    mount_irq_handler(32, handle_timer_interrupt);
    mount_irq_handler(128, syscall_trampoline);
    mount_irq_handler(250, context_switch);
}