#include "gdt.h"

const segment_descriptor gdt_data[5] = {
    {},
    gen_code_segment_descriptor(0, 0xFFFFF, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0),
    gen_data_segment_descriptor(0, 0xFFFFF, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0),
    gen_code_segment_descriptor(0, 0xFFFFF, 1, 1, 0, 0, 1, 3, 1, 0, 0, 0),
    gen_data_segment_descriptor(0, 0xFFFFF, 1, 1, 0, 0, 1, 3, 1, 0, 1, 0)
};

const gdt_descriptor gdt = {
    .data = gdt_data,
    .limit = sizeof(gdt_data) - 1,
};
