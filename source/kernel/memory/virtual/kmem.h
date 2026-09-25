#ifndef SOS_KMEM_H
#define SOS_KMEM_H

#include "../../lib/types.h"
#include "umem.h"

/*
 * Routines to safely exchange data between kernel/kernel buffers, or
 * user/kernel buffers. Pointers marked as __user might come from both kernel or
 * userspace.
 */
bool copy_to(void* __user dst, void* src, u64 length);

bool copy_from(void* dst, void* __user src, u64 length);

string copy_string_from(string __user src, u64 limit);

#endif // SOS_KMEM_H
