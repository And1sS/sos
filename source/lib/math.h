#ifndef SOS_MATH_H
#define SOS_MATH_H

#include "types.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

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

#endif // SOS_MATH_H
