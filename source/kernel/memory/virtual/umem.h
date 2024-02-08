#ifndef SOS_UMEM_H
#define SOS_UMEM_H

#include "../../lib/types.h"
#include "vm.h"
#include "vmm.h"

/*
 * Routines to safely copy from/write to user space
 */
#define __user

bool copy_to_user(void* __user dst, void* src, u64 length);

bool copy_from_user(void* dst, void* __user src, u64 length);

#endif // SOS_UMEM_H
