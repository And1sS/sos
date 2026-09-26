#include "kmem.h"
#include "../../lib/string.h"

bool copy_to(void* __user dst, void* src, u64 length) {
    if ((u64) dst < USER_SPACE_END_VADDR)
        return copy_to_user(dst, src, length);

    memcpy(dst, src, length);
    return true;
}

bool copy_from(void* dst, void* __user src, u64 length) {
    if ((u64) src < USER_SPACE_END_VADDR)
        return copy_from_user(dst, src, length);

    memcpy(dst, src, length);
    return true;
}

string copy_string_from(string __user src, u64 limit) {
    if ((u64) src < USER_SPACE_END_VADDR)
        return copy_string_from_user(src, limit);

    return strcpyn(src, limit);
}