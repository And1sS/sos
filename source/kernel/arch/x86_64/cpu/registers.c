#include "registers.h"

u64 get_cr2() {
    u64 cr2;

    __asm__ volatile("mov %%cr2, %0" : "=rm"(cr2) : : "memory");

    return cr2;
}