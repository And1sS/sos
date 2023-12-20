#include "../../../lib/types.h"

u64 get_rflags() {
    u64 rflags;

    __asm__ volatile("pushf\n\t"
                     "pop %0"
                     : "=rm"(rflags)
                     :
                     : "memory");

    return rflags;
}