#include "../../arch_init.h"
#include "../../scheduler/scheduler.h"
#include "cpu/gdt.h"
#include "interrupts/idt.h"
#include "interrupts/isrs.h"
#include "interrupts/pic.h"
#include "memory/memory_init.h"
#include "timer/pit.h"

void arch_init(const multiboot_info* const mboot_info) {
    gdt_init();

    idt_init();
    pic_init();
    pit_init();

    memory_init(mboot_info);

    threading_init();
    scheduler_init();

    mount_irq_handler(32, context_switch);
    mount_irq_handler(250, context_switch);
}

