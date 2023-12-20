#include "memory_util.h"

// TODO: optimize these procedures, so we don't copy byte by byte, but copy
//       whole cache lines instead

void* memset(void* dst, u8 val, u64 len) {
    u8* _dst = (u8*) dst;

    for (u64 i = 0; i < len; i++) {
        _dst[i] = val;
    }

    return dst;
}

void memcpy(void* dst, void* src, u64 len) {
    u8* _dst = (u8*) dst;
    u8* _src = (u8*) src;

    for (u64 i = 0; i < len; i++) {
        _dst[i] = _src[i];
    }
}