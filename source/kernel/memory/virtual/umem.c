#include "umem.h"

bool copy_to_user(void* __user dst, void* src, u64 length) {
    vm_space* current_vm = vmm_current_vm_space();
    rw_spin_lock_write_irq(&current_vm->lock);
    vm_area* surrounding_area =
        vm_space_get_surrounding_area(current_vm, (u64) dst, length);

    if (!surrounding_area || !surrounding_area->flags.writable) {
        rw_spin_unlock_write_irq(&current_vm->lock);
        return false;
    }

    memcpy(dst, src, length);
    rw_spin_unlock_write_irq(&current_vm->lock);
    return true;
}

bool copy_from_user(void* dst, void* __user src, u64 length) {
    vm_space* current_vm = vmm_current_vm_space();
    rw_spin_lock_write_irq(&current_vm->lock);
    vm_area* surrounding_area =
        vm_space_get_surrounding_area(current_vm, (u64) src, length);

    if (!surrounding_area) {
        rw_spin_unlock_write_irq(&current_vm->lock);
        return false;
    }

    memcpy(dst, src, length);
    rw_spin_unlock_write_irq(&current_vm->lock);
    return true;
}