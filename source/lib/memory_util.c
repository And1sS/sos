#include "memory_util.h"

void* memset(void* dst, u8 val, u64 len) {
    u8* _dst = (u8*) dst;
    u8* _rem_dst = _dst + len % sizeof(u64);

    while (_dst < _rem_dst) {
        *_dst++ = val;
    }

    u16 wval = ((u16) val << 8) | val;
    u32 dwval = ((u32) wval << 16) | wval;
    u64 qwval = ((u64) dwval << 32) | dwval;

    u64* _ldst = (u64*) _dst;
    u64* end = (u64*) ((u64) dst + len);
    while (_ldst < end) {
        *_ldst++ = qwval;
    }

    return dst;
}

// TODO: optimize this, so we don't copy byte by byte, but copy whole cache
//       lines instead
void memcpy(void* dst, void* src, u64 len) {
    u8* _dst = (u8*) dst;
    u8* _src = (u8*) src;

    for (u64 i = 0; i < len; i++) {
        _dst[i] = _src[i];
    }
}