#include "../../arch_init.h"
#include "../../scheduler/scheduler.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "memory/memory_init.h"
#include "timer/pit.h"

void arch_init(const multiboot_info* const mboot_info) {
    init_gdt();
    init_memory(mboot_info);
    pit_init();
    init_idt();

    threading_init();
    scheduler_init();
}
