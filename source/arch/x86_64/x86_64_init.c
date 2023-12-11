#include "../../arch_init.h"
#include "../../scheduler/scheduler.h"
#include "../../scheduler/thread_cleaner.h"
#include "cpu/gdt.h"
#include "interrupts/idt.h"
#include "memory/memory_init.h"
#include "timer/pit.h"

void arch_init(const multiboot_info* const mboot_info) {
    gdt_init();
    idt_init();
    memory_init(mboot_info);
    pit_init();

    threading_init();
    scheduler_init();
    thread_cleaner_init();
}
