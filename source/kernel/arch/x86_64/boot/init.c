#include "../../common/init.h"
#include "../../../memory/physical/pmm.h"
#include "../cpu/features.h"
#include "../cpu/gdt.h"
#include "../cpu/tss.h"
#include "../interrupts/interrupts.h"
#include "pmm_init.h"

void arch_init(const multiboot_info* const mboot_info) {
    gdt_init();
    tss_init();
    interrupts_init();
    pmm_init(mboot_info);

    print("Finished memory mapping! Free frames: ");
    print_u64(pmm_frames_available());
    println("");

    features_init();
}
