#ifndef GDT_H
#define GDT_H

#include "types.h"

#define SEGMENT_PRESENT_OFFSET 7
#define DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET 5
#define DESCRIPTOR_TYPE_OFFSET 4
#define DATA_OR_CODE_OFFSET 3
#define EXPANSION_OFFSET 2
#define CONFORMING_OFFSET 2
#define WRITE_ENABLE_OFFSET 1
#define READ_ENABLE_OFFSET 1
#define ACCESSED_OFFSET 0

#define gen_code_segment_descriptor_8_15_flags(                                \
    present, dpl, descriptor_type, conforming, read_enabled, accessed)         \
    ((((present) &1) << SEGMENT_PRESENT_OFFSET)                                \
     | (((dpl) &0b11) << DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET)                    \
     | (((descriptor_type) &1) << DESCRIPTOR_TYPE_OFFSET)                      \
     | (1 << DATA_OR_CODE_OFFSET) | (((conforming) &1) << CONFORMING_OFFSET)   \
     | (((read_enabled) &1) << READ_ENABLE_OFFSET)                             \
     | (((accessed) &1) << ACCESSED_OFFSET))

#define gen_data_segment_descriptor_8_15_flags(present, dpl, descriptor_type,  \
                                               expansion_direction,            \
                                               write_enabled, accessed)        \
    ((((present) &1) << SEGMENT_PRESENT_OFFSET)                                \
     | (((dpl) &0b11) << DESCRIPTOR_PRIVILEGE_LEVEL_OFFSET)                    \
     | (((descriptor_type) &1) << DESCRIPTOR_TYPE_OFFSET)                      \
     | (0 << DATA_OR_CODE_OFFSET)                                              \
     | (((expansion_direction) &1) << EXPANSION_OFFSET)                        \
     | (((write_enabled) &1) << WRITE_ENABLE_OFFSET)                           \
     | (((accessed) &1) << ACCESSED_OFFSET))

#define GRANULARITY_OFFSET 7
#define DEFAULT_OPERATION_SIZE_OFFSET 6
#define LONG_MODE_SEGMENT_OFFSET 5
#define AVAILABLE_FOR_SYSTEM_OFFSET 4
#define SEGMENT_LIMIT_16_19_OFFSET 0

#define gen_segment_descriptor_16_23_flags(                                    \
    granularity, default_operation_size, long_mode, available_for_system,      \
    segment_limit_16_23)                                                       \
    (((granularity) << GRANULARITY_OFFSET)                                     \
     | ((default_operation_size) << DEFAULT_OPERATION_SIZE_OFFSET)             \
     | ((long_mode) << LONG_MODE_SEGMENT_OFFSET)                               \
     | ((available_for_system) << AVAILABLE_FOR_SYSTEM_OFFSET)                 \
     | ((segment_limit_16_23) << SEGMENT_LIMIT_16_19_OFFSET))

typedef struct __attribute__((__aligned__(8), __packed__)) {
    u16 segment_limit_0_15;
    u32 base_addr_0_23 : 24;
    u8 flags_8_15;
    u8 flags_16_23;
    u8 base_addr_24_31;
} segment_descriptor;

#define gen_code_segment_descriptor(                                           \
    base_addr, segment_limit, granularity, default_operation_size, long_mode,  \
    available_for_system, present, dpl, descriptor_type, conforming,           \
    read_enabled, accessed)                                                    \
    {                                                                          \
        .segment_limit_0_15 = segment_limit & 0xFFFF,                          \
        .base_addr_0_23 = base_addr & 0xFFFFFF,                                \
        .flags_8_15 = gen_code_segment_descriptor_8_15_flags(                  \
            present, dpl, descriptor_type, conforming, read_enabled,           \
            accessed),                                                         \
        .flags_16_23 = gen_segment_descriptor_16_23_flags(                     \
            granularity, default_operation_size, long_mode,                    \
            available_for_system, (segment_limit >> 16) & 0xF),                \
        .base_addr_24_31 = (base_addr >> 24) & 0xFF                            \
    }

#define gen_data_segment_descriptor(                                           \
    base_addr, segment_limit, granularity, default_operation_size, long_mode,  \
    available_for_system, present, dpl, descriptor_type, expansion_direction,  \
    write_enabled, accessed)                                                   \
    {                                                                          \
        .segment_limit_0_15 = segment_limit & 0xFFFF,                          \
        .base_addr_0_23 = base_addr & 0xFFFFFF,                                \
        .flags_8_15 = gen_data_segment_descriptor_8_15_flags(                  \
            present, dpl, descriptor_type, expansion_direction, write_enabled, \
            accessed),                                                         \
        .flags_16_23 = gen_segment_descriptor_16_23_flags(                     \
            granularity, default_operation_size, long_mode,                    \
            available_for_system, (segment_limit >> 16) & 0xF),                \
        .base_addr_24_31 = (base_addr >> 24) & 0xFF                            \
    }

typedef struct __attribute__((__packed__)) {
    const u16 limit;
    const segment_descriptor* data;
} gdt_descriptor;

extern const segment_descriptor gdt_data[5];
extern const gdt_descriptor gdt;

extern void load_gdt(const gdt_descriptor* gdt);

#endif // GDT_H
