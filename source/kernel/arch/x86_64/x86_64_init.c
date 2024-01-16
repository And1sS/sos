#include "../../boot/arch_init.h"
#include "../../memory/pmm.h"
#include "boot/x86_64_pmm_init.h"
#include "cpu/gdt.h"
#include "cpu/tss.h"
#include "interrupts/idt.h"
#include "timer/pit.h"

void arch_init(const multiboot_info* const mboot_info) {
    gdt_init();
    tss_init();
    idt_init();
    pit_init();

    pmm_init(mboot_info);

    print("Finished memory mapping! Free frames: ");
    print_u64(get_available_frames_count());
    println("");
}
