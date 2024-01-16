#include "../../../memory/pmm.h"
#include "../memory/paging.h"

/*
 * Paging initialization:
 *
 * While running boot code without paging at 0x100000:
 * 1) set up identity mapping of first 2MiB RAM to (0 - 0x200000) address space
 * In second megabyte of RAM lives boot code that is used for short time after
 * paging enablement. Code that jumps to higher half trampoline also lives here.
 * Page size used: 4KiB
 * Performed in entry.asm
 *
 *
 * 2) set up identity mapping of first 2MiB RAM to
 * (0XFFFF888000000000 - 0XFFFF888000200000) address space
 * In second megabyte of ram specifically lives trampoline to higher half, so
 * this mapping makes it usable.
 * Page size used: 4KiB
 * Performed in entry.asm
 *
 *
 * 3) set up identity mapping of first 1GiB RAM to
 * kernel space (0xFFFF800000000000 - 0xFFFF800040000000)
 * This mapping is temporary and is used to be able to run early boot kernel
 * space code and use kernel space data without knowing kernel size in RAM. It
 * maps only 1GiB because this mapping is performed from 32 bit mode that can
 * operate only with 4GiB addresses. Later, whole kernel space (512GiB) will be
 * remapped.
 * Page size used: 2MiB(Huge pages)
 * Performed in entry.asm
 *
 *
 * Then paging is enabled and jump to higher half code (0xFFFF800000000000) is
 * performed, after that:
 * 4) set up whole kernel space identity mapping
 * (0xFFFF800000000000 - 0xFFFF87FFFFFFFFFF) This mapping runs in 64 bit mode
 * and maps 512GiB of (potential) RAM to kernel space. This is used to be able
 * to run any kernel code without knowing kernel size in RAM. Page size used:
 * 1GiB(Huge pages) Performed in entry.asm
 *
 *
 * 5) set up identity mapping of all (potential) RAM to kernel space
 * (0xFFFF888000000000 - 0xFFFFC87FFFFFFFFF).
 * This mapping runs in 64 bit mode and maps 64TB of (potential) RAM to kernel
 * space. This is used to be able to write to/read from any physical ram page.
 * (Particularly used to parse multiboot structure).
 * Page size used: 1GiB(Huge pages)
 * Performed in early_paging.c (this file)
 */

#define HUGE_PAGE_SIZE_1GB 0x40000000


/*
 * Early stage ram identity mapping to kernel address space.
 * This function sets up mapping for 64TB of potential ram, to be able to parse
 * memory map. Later, after actual ram size is defined, mapping of non-existent
 * ram should be removed.
 *
 * Note: pml3_pool address is virtual inside kernel bss section
 * (defined in entry.asm), so when inserting into pml4 it has to be adjusted
 * by start address of kernel
 */
void __attribute__((section(".boot64_text")))
set_up_64tb_ram_identity_mapping(page_table* pml4, page_table pml3_pool[128]) {

    paddr frame = 0;
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < PT_ENTRIES; j++) {
            pml3_pool[i].entries[j] =
                frame | PRESENT_ATTR | RW_ATTR | HUGE_PAGE_ATTR;
            frame += HUGE_PAGE_SIZE_1GB;
        }

        pml4->entries[KERNEL_VMAPPED_RAM_P4_START_ENTRY + i] =
            (((u64) &pml3_pool[i]) - KERNEL_START_VADDR) | PRESENT_ATTR
            | RW_ATTR;
    }
}