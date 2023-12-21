#include "../../boot/arch_init.h"
#include "../../scheduler/scheduler.h"
#include "../../threading/threading.h"
#include "cpu/gdt.h"
#include "cpu/tss.h"
#include "interrupts/idt.h"
#include "memory/memory_init.h"
#include "timer/pit.h"

void arch_init(const multiboot_info* const mboot_info) {
    gdt_init();
    tss_set_up();
    idt_init();
    memory_init(mboot_info);
    pit_init();

    threading_init();
    scheduler_init();
}
