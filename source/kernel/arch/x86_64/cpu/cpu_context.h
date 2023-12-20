#ifndef SOS_CPU_CONTEXT_H
#define SOS_CPU_CONTEXT_H

#include "../../../lib/types.h"

typedef struct __attribute__((__packed__)) {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} cpu_context;

#endif // SOS_CPU_CONTEXT_H
