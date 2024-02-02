#ifndef SOS_VMM_H
#define SOS_VMM_H

#include "../../memory/memory_map.h"
#include "../../memory/virtual/vm.h"

extern const u64 PAGE_SIZE;

void arch_init_kernel_vm(vm_space* kernel_space);

void arch_set_vm_space(vm_space* space);
void arch_notify_vm_space_changed();

bool arch_map_page(struct page_table* table, vaddr page, vm_area_flags flags);
bool arch_map_page_to_frame(struct page_table* table, vaddr page, paddr frame,
                            vm_area_flags flags);
bool arch_unmap_page(struct page_table* table, vaddr page);

bool arch_map_kernel_page(vaddr page, vm_area_flags flags);

void* arch_get_page_view(struct page_table* table, vaddr page);
vm_area_flags arch_get_page_flags(struct page_table* table, vaddr page);

struct page_table* arch_fork_page_table(struct page_table* table);
void arch_destroy_page_table(struct page_table* table);

#endif // SOS_VMM_H
