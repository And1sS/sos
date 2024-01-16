#include "x86_64_pmm_init.h"
#include "../../../memory/pmm.h"

#define IS_INSIDE(frame, start, end)                                           \
    (((frame) >= (start)) && ((frame) <= (end)))

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

void pmm_init(const multiboot_info* const mboot_info) {
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