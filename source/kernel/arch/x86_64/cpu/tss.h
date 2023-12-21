#ifndef SOS_TSS_H
#define SOS_TSS_H

#include "../../../lib/types.h"

typedef struct __attribute__((__packed__)) {
    u32 reserved_0;
    u32 rsp0_low;
    u32 rsp0_high;
    u32 rsp1_low;
    u32 rsp1_high;
    u32 rsp2_low;
    u32 rsp2_high;
    u64 reserved_1;
    u32 ist1_low;
    u32 ist1_high;
    u32 ist2_low;
    u32 ist2_high;
    u32 ist3_low;
    u32 ist3_high;
    u32 ist4_low;
    u32 ist4_high;
    u32 ist5_low;
    u32 ist5_high;
    u32 ist6_low;
    u32 ist6_high;
    u32 ist7_low;
    u32 ist7_high;
    u64 reserved_2;
    u16 reserved_3;
    u16 iopb;
} task_state_segment;

extern task_state_segment tss;

void tss_set_up();

#endif // SOS_TSS_H
