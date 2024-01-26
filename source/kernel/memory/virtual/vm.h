#ifndef SOS_VM_SPACE_H
#define SOS_VM_SPACE_H

#include "../../lib/container/array_list/array_list.h"
#include "../../lib/ref_count/ref_count.h"
#include "../../synchronization/rw_spin_lock.h"
#include "../../synchronization/spin_lock.h"
#include "../memory_map.h"

struct page_table;

typedef struct {
    bool present : 1;
    bool writable : 1;
    bool user_access_allowed : 1;
    bool executable : 1;
    bool shared : 1;
} vm_area_flags;

/*
 * All fields are guarded with owning vm_space's lock
 */
typedef struct {
    vaddr base;
    u64 length;
    vm_area_flags flags;
} vm_area;

/*
 * All instances of this struct hold (and own) lower half of virtual address
 * space (user space) and its areas, except single kernel instance (which can be
 * obtained via vmm_kernel_vm_space()) that holds and owns higher half of
 * virtual address space and its mapped areas.
 *
 * This means that `vm_space_fork` clones entire user half of virtual
 * memory space (page table hierarchy and underlying pages) and
 * `vm_space_destroy` destroys lower half of provided virtual address space
 * (table hierarchy underlying pages). In the future, this may be changed in
 * favor of reference counting and Copy on write, so that we don't copy shared
 * physical pages that are only read from, but not written to.
 */
typedef struct {
    bool is_kernel_space;
    struct page_table* table;
    array_list areas;
    ref_count refc;
    rw_spin_lock lock;
} vm_space;

/*
 * Panics in case something goes wrong. Public because arch needs way to insert
 * areas it created in kernel space during boot process.
 */
bool vm_space_insert_area_unsafe(vm_space* space, vm_area* to_insert);

vm_space* vm_space_fork(vm_space* space);
void vm_space_destroy(vm_space* space);

typedef enum {
    SUCCESS = 0,
    ALREADY_MAPPED = 1,
    OUT_OF_MEMORY = 2
} vm_page_mapping_result;

typedef struct {
    u64 mapped;
    vm_page_mapping_result status;
} vm_pages_mapping_result;

vm_page_mapping_result vm_space_map_page(vm_space* space, vaddr base,
                                         vm_area_flags flags);
vm_pages_mapping_result vm_space_map_pages(vm_space* space, vaddr base,
                                           u64 count, vm_area_flags flags);

void* vm_space_get_page_view(vm_space* space, vaddr base);

bool vm_space_unmap_page(vm_space* space, vaddr base);
bool vm_space_unmap_pages(vm_space* space, vaddr base, u64 count);

void vm_space_print(vm_space* space);

#endif // SOS_VM_SPACE_H
