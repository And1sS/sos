#include "math.h"

i8 msb_u32(u32 value) {
    if (value == 0) {
        return -1;
    }

    return 32 - __builtin_clz(value);
}

i8 msb_u64(u64 value) {
    if (value == 0) {
        return -1;
    }

    u32 top = (value >> 32) & 0xFFFFFFFF;
    u32 bottom = value & 0xFFFFFFFF;

    i8 top_msb = msb_u32(top);
    if (top_msb != -1) {
        top_msb += 32;
    }

    i8 bottom_msb = msb_u32(bottom);

    return MAX(top_msb, bottom_msb);
}

i8 lsb_u32(u32 value) {
    if (value == 0) {
        return -1;
    }

    return __builtin_ctz(value);
}

i8 lsb_u64(u64 value) {
    if (value == 0) {
        return -1;
    }

    u32 top = (value >> 32) & 0xFFFFFFFF;
    u32 bottom = value & 0xFFFFFFFF;

    if (bottom > 0) {
        return lsb_u32(bottom);
    }

    return lsb_u32(top) + 32;
}