#include "../../arch_init.h"
#include "../../scheduler/scheduler.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "memory/memory_init.h"
#include "timer/timer.h"

void arch_init(const multiboot_info* const mboot_info) {
    init_gdt();
    init_memory(mboot_info);
    init_timer();
    init_idt();

    threading_init();
    scheduler_init();
}
