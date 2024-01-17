#include "vmm.h"
#include "../../arch/common/vmm.h"
#include "vm.h"

vm_space kernel_vm_space;

// TODO: make this percpu
// can be changed without locking as this is confined to current cpu
static vm_space* current_vm_space = NULL;

void vmm_init() {
    arch_init_kernel_vm(&kernel_vm_space);
    kernel_vm_space.is_kernel_space = true;
    kernel_vm_space.refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    kernel_vm_space.lock = (rw_spin_lock) RW_LOCK_STATIC_INITIALIZER;
    current_vm_space = &kernel_vm_space;
}

vm_space* vmm_kernel_vm_space() { return &kernel_vm_space; }

vm_space* vmm_current_vm_space() { return current_vm_space; }

void vmm_set_vm_space(vm_space* space) {
    atomic_set((u64*) current_vm_space, (u64) space);
    arch_set_vm_space(space);
    // no need to call notify here, since arch already knows that vm space has
    // changed
}

void vmm_notify_vm_space_changed() {
    // TODO: when implementing support for SMP make this to send IPI to check
    //       whether other CPUs need to also get notified
    arch_notify_vm_space_changed();
}