#include "../../../memory/virtual/vmm.h"
#include "../../../memory/heap/kheap.h"
#include "../../../memory/physical/pmm.h"
#include "../cpu/features.h"
#include "paging.h"

#define TABLE(entry) ((page_table*) P2V(MASK_FLAGS(entry)))
#define PAGE(entry) ((void*) P2V(MASK_FLAGS(entry)))

const u64 PAGE_SIZE = 4096;

static string PREALLOCATION_ERROR_MSG =
    "Could not initialize virtual memory manager: not enough "
    "memory for kernel page table pre-allocation";

static void flush_tlb() {
    __asm__ volatile("push %%rax\n"
                     "mov %%cr3, %%rax\n"
                     "mov %%rax, %%cr3\n"
                     "pop %%rax"
                     :
                     :
                     : "memory");
}

static void populate_kernel_pml4_with_kernel_entries() {
    for (u16 i = PT_ENTRIES / 2; i < PT_ENTRIES; i++) {
        if (!(kernel_p4_table.entries[i] & PRESENT_ATTR)) {
            paddr frame = pmm_allocate_zeroed_frame();
            if (!frame)
                panic(PREALLOCATION_ERROR_MSG);

            kernel_p4_table.entries[i] = frame | PRESENT_ATTR | WRITABLE_ATTR;
        }
    }

    flush_tlb();
}

static vm_area* create_kernel_binary_vm_area() {
    vm_area* kernel_binary_area = (vm_area*) kmalloc(sizeof(vm_area));
    if (!kernel_binary_area)
        panic(PREALLOCATION_ERROR_MSG);

    kernel_binary_area->base = KERNEL_START_VADDR;
    kernel_binary_area->length = (u64) 512 * 1024 * 1024 * 1024; // 512GiB
    kernel_binary_area->flags = (vm_area_flags){.writable = true,
                                                .present = true,
                                                .executable = true,
                                                .shared = true,
                                                .user_access_allowed = false};

    return kernel_binary_area;
}

static vm_area* create_kernel_vmapped_ram_vm_area() {
    vm_area* kernel_vmapped_ram_area = (vm_area*) kmalloc(sizeof(vm_area));
    if (!kernel_vmapped_ram_area)
        panic(PREALLOCATION_ERROR_MSG);

    kernel_vmapped_ram_area->base = KERNEL_VMAPPED_RAM_START_VADDR;
    kernel_vmapped_ram_area->length =
        (u64) 64 * 1024 * 1024 * 1024 * 1024; // 64 TiB
    kernel_vmapped_ram_area->flags =
        (vm_area_flags){.writable = true,
                        .present = true,
                        .executable = true,
                        .shared = true,
                        .user_access_allowed = false};

    return kernel_vmapped_ram_area;
}

static vm_area* create_kernel_heap_vm_area() {
    vm_area* kernel_heap_vm_area = (vm_area*) kmalloc(sizeof(vm_area));
    if (!kernel_heap_vm_area)
        panic(PREALLOCATION_ERROR_MSG);

    kernel_heap_vm_area->base = KHEAP_START_VADDR;
    kernel_heap_vm_area->length = kheap_size();
    kernel_heap_vm_area->flags = (vm_area_flags){.writable = true,
                                                 .present = true,
                                                 .executable = true,
                                                 .shared = true,
                                                 .user_access_allowed = false};

    return kernel_heap_vm_area;
}

/*
 * Populates kernel page table, so that pml4 will never change and shouldn't be
 * synchronized across memory spaces
 */
void arch_init_kernel_vm(vm_space* kernel_space) {
    populate_kernel_pml4_with_kernel_entries();

    // kernel page table resides inside mapped kernel binary space,
    // this will give the rest of the kernel its view inside vmapped ram space
    kernel_space->table =
        (struct page_table*) P2V((u64) &kernel_p4_table - KERNEL_START_VADDR);

    if (!array_list_init(&kernel_space->areas, 8)) {
        panic(PREALLOCATION_ERROR_MSG);
    }

    bool kernel_area_inserted = vm_space_insert_area_unsafe(
        kernel_space, create_kernel_binary_vm_area());
    bool vmapped_ram_area_inserted = vm_space_insert_area_unsafe(
        kernel_space, create_kernel_vmapped_ram_vm_area());
    bool kheap_area_inserted =
        vm_space_insert_area_unsafe(kernel_space, create_kernel_heap_vm_area());

    if (!kernel_area_inserted || !vmapped_ram_area_inserted || !kheap_area_inserted)
        panic("Could not kreate initial kernel space areas");
}

static u64 vm_area_flags_to_x86_64_flags(vm_area_flags flags) {
    u64 result = 0;
    result |= flags.present ? PRESENT_ATTR : 0;
    result |= flags.writable ? WRITABLE_ATTR : 0;
    result |= flags.user_access_allowed ? SUPERVISOR_ATTR : 0;
    result |= !flags.executable && features_execute_disable_supported()
                  ? EXECUTE_DISABLE_ATTR
                  : 0;
    return result;
}

// TODO: think about what to do if page address is not PAGE_SIZE aligned
void* arch_get_page_view(struct page_table* table, vaddr page) {
    if (!IS_CANONICAL(page))
        return NULL;

    page_table* arch_table = (page_table*) table;
    page = PAGE_ALIGN(page);
    u64 pml3 = arch_table->entries[P4_OFFSET(page)];
    if (!(pml3 & PRESENT_ATTR))
        return NULL;

    u64 pml2 = NEXT_PTE(pml3, 3, page);
    if (!(pml2 & PRESENT_ATTR))
        return NULL;

    u64 pml1 = NEXT_PTE(pml2, 2, page);
    if (!(pml1 & PRESENT_ATTR))
        return NULL;

    return PAGE(NEXT_PTE(pml1, 1, page));
}

static bool map_page(page_table* table, vaddr page, paddr frame, u64 flags) {
    if (!IS_CANONICAL(page))
        return false;

    flags &= FLAGS_MASK;
    page = PAGE_ALIGN(page);
    frame = MASK_FLAGS(frame);

    u64 pml4_entry = table->entries[P4_OFFSET(page)];
    if (!(pml4_entry & PRESENT_ATTR)) {
        paddr pml3 = pmm_allocate_zeroed_frame();
        if (!pml3)
            return false;

        table->entries[P4_OFFSET(page)] = pml4_entry = pml3 | flags;
    }

    u64 pml3_entry = NEXT_PTE(pml4_entry, 3, page);
    if (!(pml3_entry & PRESENT_ATTR)) {
        paddr pml2 = pmm_allocate_zeroed_frame();
        if (!pml2)
            return false;

        NEXT_PTE(pml4_entry, 3, page) = pml3_entry = pml2 | flags;
    }

    u64 pml2_entry = NEXT_PTE(pml3_entry, 2, page);
    if (!(pml2_entry & PRESENT_ATTR)) {
        paddr pml1 = pmm_allocate_zeroed_frame();
        if (!pml1)
            return false;

        NEXT_PTE(pml3_entry, 2, page) = pml2_entry = pml1 | flags;
    }

    NEXT_PTE(pml2_entry, 1, page) = frame | flags;
    return true;
}

static void destroy_pml1(paddr pml1) {
    page_table* table = TABLE(pml1);
    for (u16 i = 0; i < PT_ENTRIES; ++i) {
        u64 entry = table->entries[i];
        if (entry & PRESENT_ATTR) {
            pmm_free_frame(MASK_FLAGS(entry));
        }
    }

    pmm_free_frame(pml1);
}

static void destroy_pml2(paddr pml2) {
    page_table* table = TABLE(pml2);
    for (u16 i = 0; i < PT_ENTRIES; ++i) {
        u64 entry = table->entries[i];
        if (entry & PRESENT_ATTR) {
            destroy_pml1(MASK_FLAGS(entry));
        }
    }
    pmm_free_frame(pml2);
}

static void destroy_pml3(paddr pml3) {
    page_table* table = TABLE(pml3);
    for (u16 i = 0; i < PT_ENTRIES; ++i) {
        u64 entry = table->entries[i];
        if (entry & PRESENT_ATTR) {
            destroy_pml2(MASK_FLAGS(entry));
        }
    }
    pmm_free_frame(pml3);
}

static void destroy_page_table(page_table* table) {
    // kernel half of address space is owned by kernel vm, so don't destroy it
    for (u16 i = 0; i < PT_ENTRIES / 2; ++i) {
        u64 entry = table->entries[i];
        if (entry & PRESENT_ATTR) {
            destroy_pml3(MASK_FLAGS(entry));
        }
    }

    pmm_free_frame(V2P(table));
}

void arch_destroy_page_table(struct page_table* table) {
    destroy_page_table((page_table*) table);
}

bool arch_map_page_to_frame(struct page_table* table, vaddr page, paddr frame,
                            vm_area_flags flags) {

    if (!IS_CANONICAL(page))
        return false;

    page_table* arch_table = (page_table*) table;
    u64 arch_flags = vm_area_flags_to_x86_64_flags(flags);
    page = PAGE_ALIGN(page);

    return map_page(arch_table, page, frame, arch_flags);
}

bool arch_map_page(struct page_table* table, vaddr page, vm_area_flags flags) {
    if (!IS_CANONICAL(page))
        return false;

    paddr frame = pmm_allocate_zeroed_frame();
    if (!frame)
        return false;

    return arch_map_page_to_frame(table, page, frame, flags);
}

bool arch_unmap_page(struct page_table* table, vaddr page) {
    if (!IS_CANONICAL(page))
        return false;

    page_table* arch_table = (page_table*) table;
    page = PAGE_ALIGN(page);
    u64 pml3 = arch_table->entries[P4_OFFSET(page)];
    if (!(pml3 & PRESENT_ATTR))
        return false;

    u64 pml2 = NEXT_PTE(pml3, 3, page);
    if (!(pml2 & PRESENT_ATTR))
        return false;

    u64 pml1 = NEXT_PTE(pml2, 2, page);
    if (!(pml1 & PRESENT_ATTR))
        return false;

    return NEXT_PTE(pml1, 1, page) = 0;
}

bool arch_map_kernel_page(vaddr page, vm_area_flags flags) {
    return arch_map_page((struct page_table*) &kernel_p4_table, page, flags);
}

static paddr clone_pml1(paddr pml1) {
    paddr cloned_pml1 = pmm_allocate_zeroed_frame();
    if (!cloned_pml1)
        return NULL;

    page_table* table = TABLE(pml1);
    page_table* cloned_table = TABLE(cloned_pml1);

    for (u16 i = 0; i < PT_ENTRIES; i++) {
        u64 pml1_entry = table->entries[i];
        if (pml1_entry & PRESENT_ATTR) {
            paddr cloned_page = pmm_allocate_zeroed_frame();
            if (!cloned_page)
                goto cleanup_cloned_table;

            memcpy(PAGE(cloned_page), PAGE(pml1_entry), PAGE_SIZE);
            cloned_table->entries[i] = cloned_page | GET_FLAGS(pml1_entry);
        }
    }

    return cloned_pml1;

cleanup_cloned_table:
    destroy_pml1(cloned_pml1);
    return NULL;
}

static paddr clone_pml2(paddr pml2) {
    paddr cloned_pml2 = pmm_allocate_zeroed_frame();
    if (!cloned_pml2)
        return NULL;

    page_table* table = TABLE(pml2);
    page_table* cloned_table = TABLE(cloned_pml2);

    for (u16 i = 0; i < PT_ENTRIES; i++) {
        u64 pml2_entry = table->entries[i];
        if (pml2_entry & PRESENT_ATTR) {
            paddr cloned_pml1 = clone_pml1(MASK_FLAGS(pml2_entry));
            if (!cloned_pml1)
                goto cleanup_cloned_table;

            cloned_table->entries[i] = cloned_pml1 | GET_FLAGS(pml2_entry);
        }
    }

    return cloned_pml2;

cleanup_cloned_table:
    destroy_pml2(cloned_pml2);
    return NULL;
}

static paddr clone_pml3(paddr pml3) {
    paddr cloned_pml3 = pmm_allocate_zeroed_frame();
    if (!cloned_pml3)
        return NULL;

    page_table* table = TABLE(pml3);
    page_table* cloned_table = TABLE(cloned_pml3);

    for (u16 i = 0; i < PT_ENTRIES; i++) {
        u64 pml3_entry = table->entries[i];
        if (pml3_entry & PRESENT_ATTR) {
            paddr cloned_pml2 = clone_pml2(MASK_FLAGS(pml3_entry));
            if (!cloned_pml2)
                goto cleanup_cloned_table;

            cloned_table->entries[i] = cloned_pml2 | GET_FLAGS(pml3_entry);
        }
    }

    return cloned_pml3;

cleanup_cloned_table:
    destroy_pml3(cloned_pml3);
    return NULL;
}

static page_table* clone_page_table(page_table* table) {
    paddr cloned_pml4 = pmm_allocate_zeroed_frame();
    if (!cloned_pml4)
        return NULL;

    page_table* cloned_table = TABLE(cloned_pml4);
    memcpy(cloned_table, table, sizeof(page_table));

    for (u16 i = 0; i < PT_ENTRIES / 2; i++) {
        u64 pml4_entry = table->entries[i];
        if (pml4_entry & PRESENT_ATTR) {
            paddr cloned_pml3 = clone_pml3(MASK_FLAGS(pml4_entry));
            if (!cloned_pml3)
                goto cleanup_cloned_table;

            cloned_table->entries[i] = cloned_pml3 | GET_FLAGS(pml4_entry);
        }
    }

    return cloned_table;

cleanup_cloned_table:
    destroy_page_table(cloned_table);
    return NULL;
}

// For now plainly copies whole page table hierarchy
// TODO: change to copy on write
struct page_table* arch_fork_page_table(struct page_table* table) {
    return (struct page_table*) clone_page_table((page_table*) table);
}

void arch_notify_vm_space_changed() { flush_tlb(); }

void arch_set_vm_space(vm_space* space) {
    if (((u64) space->table < (u64) KERNEL_VMAPPED_RAM_START_VADDR)
        || ((u64) space->table > (u64) KERNEL_VMAPPED_RAM_END_VADDR))
        panic("Trying to set page table with invalid virtual address");

    __asm__ volatile("mov %0, %%cr3" : : "r"(V2P(space->table)));
}
