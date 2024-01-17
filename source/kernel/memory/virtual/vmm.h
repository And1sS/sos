#ifndef SOS_VIRTUAL_MEMORY_MANAGER_H
#define SOS_VIRTUAL_MEMORY_MANAGER_H

#include "../memory_map.h"
#include "vm.h"

void vmm_init();

vm_space* vmm_kernel_vm_space();
vm_space* vmm_current_vm_space();

void vmm_set_vm_space(vm_space* space);
void vmm_notify_vm_space_changed();

bool vmm_invalidate_range(vaddr base, u64 len);

bool vmm_map_kernel_range(vaddr base, u64 len, vm_area_flags flags);
bool vmm_unmap_kernel_range(vaddr base, u64 len);

#endif // SOS_VIRTUAL_MEMORY_MANAGER_H
