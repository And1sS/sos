#include "umem.h"

#include "../../error/error.h"
#include "../../error/errno.h"
#include "../../lib/math.h"
#include "../../lib/string.h"

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

string copy_string_from_user(string __user src, u64 limit) {
    vm_space* current_vm = vmm_current_vm_space();
    rw_spin_lock_read_irq(&current_vm->lock);

    // area has to be of at least length of 1
    vm_area* surrounding_area =
        vm_space_get_surrounding_area(current_vm, (u64) src, 1);

    if (!surrounding_area) {
        rw_spin_unlock_read_irq(&current_vm->lock);
        return ERROR_PTR(-EFAULT);
    }

    u64 start = (u64) src;
    u64 end = MIN(start + limit,
                  surrounding_area->base + surrounding_area->length);

    string result = strcpyn(src, end - start);
    rw_spin_unlock_read_irq(&current_vm->lock);

    return result;
}