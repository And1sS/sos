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

#define IS_INSIDE(frame, start, end)                                           \
    (((frame) >= (start)) && ((frame) <= (end)))

#define HUGE_PAGE_SIZE_1GB 0x40000000

static paddr kernel_start = NULL;
static paddr kernel_end = NULL;
static paddr mboot_start = NULL;
static paddr mboot_end = NULL;

static paddr find_end_of_memory(const multiboot_info* mboot_info);
static paddr find_kernel_start(const multiboot_info* mboot_info);
static paddr find_kernel_end(const multiboot_info* mboot_info);
static bool is_frame_available(const multiboot_info* mboot_info, paddr frame);
static bool is_inside_module(const multiboot_info* mboot_info, paddr frame);
static void pmm_maybe_free_frame(const multiboot_info* mboot_info, paddr frame);

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
void set_up_64tb_ram_identity_mapping(page_table* pml4,
                                      page_table pml3_pool[128]) {

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

void identity_map_ram(const multiboot_info* const mboot_info) {
    kernel_start = find_kernel_start(mboot_info);
    kernel_end = find_kernel_end(mboot_info);
    mboot_start = mboot_info->original_struct_addr;
    mboot_end = mboot_start + mboot_info->size;

    paddr memory_end = find_end_of_memory(mboot_info);

    for (paddr frame = 0; frame < memory_end; frame += FRAME_SIZE) {
        pmm_maybe_free_frame(mboot_info, frame);
    }
}

static void pmm_maybe_free_frame(const multiboot_info* const mboot_info,
                                 paddr frame) {

    // TODO: Change inclusion check to intersection check, because for now
    //       this code relies on fact that grub places kernel, multiboot
    //       struct and modules on addresses aligned on page size
    if (!is_frame_available(mboot_info, frame))
        return;
    if (IS_INSIDE(frame, kernel_start, kernel_end))
        return;
    if (IS_INSIDE(frame, mboot_start, mboot_end))
        return;
    if (is_inside_module(mboot_info, frame))
        return;

    if (frame >= RESERVED_LOWER_PMEM_SIZE)
        free_frame(frame);
}

static paddr normalize_kernel_addr(vaddr addr) {
    if (addr < KERNEL_START_VADDR)
        return addr;
    return addr - KERNEL_START_VADDR;
}

static paddr find_end_of_memory(const multiboot_info* const mboot_info) {
    paddr memory_end = 0;

    for (u32 i = 0; i < mboot_info->mmap.entries_count; ++i) {
        memory_map_entry_v0* entry = &mboot_info->mmap.entries[i];
        if (entry->type != 1)
            continue;
        paddr entry_end = entry->base_addr + entry->length - 1;
        if (entry_end > memory_end)
            memory_end = entry_end;
    }

    return memory_end;
}

static paddr find_kernel_start(const multiboot_info* mboot_info) {
    paddr kernel_start = 0xFFFFFFFFFFFFFFFF;

    for (u32 i = 1; i < mboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &mboot_info->elf_sections.sections[i];
        // kernel sections are actually mix of virtual and physical addresses
        paddr section_addr = normalize_kernel_addr(section->addr);

        if (section_addr < kernel_start)
            kernel_start = section_addr;
    }

    return kernel_start;
}

static paddr find_kernel_end(const multiboot_info* mboot_info) {
    paddr kernel_end = 0;

    for (u32 i = 1; i < mboot_info->elf_sections.sections_number; ++i) {
        elf64_shdr* section = &mboot_info->elf_sections.sections[i];
        paddr section_addr = normalize_kernel_addr(section->addr);

        // kernel sections are actually mix of virtual and physical addresses
        if (section_addr + section->size > kernel_end)
            kernel_end = section_addr + section->size;
    }

    return kernel_end;
}

static bool is_frame_available(const multiboot_info* mboot_info, paddr frame) {
    for (u32 i = 0; i < mboot_info->mmap.entries_count; ++i) {
        memory_map_entry_v0* entry = &mboot_info->mmap.entries[i];
        u64 entry_start = entry->base_addr;
        u64 entry_end = entry_start + entry->length - 1;

        if (IS_INSIDE(frame, entry_start, entry_end) && entry->type == 1) {
            return true;
        }
    }

    return false;
}

static bool is_inside_module(const multiboot_info* mboot_info, paddr frame) {
    for (u64 i = 0; i < mboot_info->modules_count; i++) {
        module mod = get_module_info(mboot_info, i);
        if (IS_INSIDE(frame, mod.mod_start, mod.mod_end)) {
            return true;
        }
    }

    return false;
}
