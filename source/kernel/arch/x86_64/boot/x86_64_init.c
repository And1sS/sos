#include "../../../boot/arch_init.h"
#include "../../../memory/pmm.h"
#include "../cpu/gdt.h"
#include "../cpu/tss.h"
#include "../interrupts/idt.h"
#include "../timer/pit.h"
#include "identity_map.h"

void arch_init(const multiboot_info* const mboot_info) {
    gdt_init();
    tss_init();
    idt_init();
    pit_init();
    identity_map_ram(mboot_info);

    print("Finished memory mapping! Free frames: ");
    print_u64(get_available_frames_count());
    println("");
}
