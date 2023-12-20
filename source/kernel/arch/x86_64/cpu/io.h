#ifndef IO_H
#define IO_H

#include "../../../lib/types.h"

static inline u8 inb(u16 addr) {
    u8 result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "d"(addr));
    return result;
}

static inline void outb(u16 addr, u8 data) {
    __asm__ volatile("outb %0, %1" : : "a"(data), "d"(addr));
}

static inline void io_wait() {
    // random unused port
    outb(0x80, 0);
}

#endif // IO_H
